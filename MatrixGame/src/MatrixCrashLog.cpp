#include "MatrixGameDll.hpp"
#include "MatrixMap.hpp"
#include "MatrixLogic.hpp"
#include "MatrixRobot.hpp"
#include "MatrixCrashLog.hpp"

#include <cstdarg>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <mutex>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <SDL.h>

#if !defined(_WIN32)
#include <execinfo.h>
#include <unistd.h>
#include <dlfcn.h>
#endif

extern const char *g_mg_stage;
extern int g_ExitState;

namespace {

constexpr int kRing = 256;
constexpr int kLine = 512;

struct Ring {
    char lines[kRing][kLine]{};
    int next = 0;
    int count = 0;
    std::mutex mu;
};

Ring g_ring;

#if defined(_WIN32)
// the VEH uses this precomputed path and avoids allocation and C++ locks
std::wstring g_crash_path_w;
void *g_veh_crash_file = nullptr;
volatile long g_veh_records = 0;
volatile long g_veh_writer = 0;
bool g_windows_veh_installed = false;
uintptr_t g_windows_module_base = 0;
extern "C" void* mgwin_create_file(const wchar_t*, uint32_t);
extern "C" int mgwin_write(void*, const void*, uint32_t, uint32_t*);
extern "C" int mgwin_close(void*);
extern "C" int mgwin_module_path(void*, wchar_t*, uint32_t);
extern "C" int mgwin_command_output(const wchar_t*, char*, uint32_t);

void append_text(char *&out, const char *text) noexcept {
    while (*text) *out++ = *text++;
}
void append_hex(char *&out, uintptr_t value) noexcept {
    static constexpr char digits[] = "0123456789ABCDEF";
    append_text(out, "0x");
    bool started = false;
    for (int shift = static_cast<int>(sizeof(value) * 8) - 4; shift >= 0; shift -= 4) {
        unsigned digit = static_cast<unsigned>((value >> shift) & 0xF);
        if (digit || started || shift == 0) { *out++ = digits[digit]; started = true; }
    }
}

std::string narrow(const std::wstring &w) {
    // preserve non-ASCII paths
    std::string out;
    out.reserve(w.size());
    for (size_t i = 0; i < w.size(); ++i) {
        uint32_t cp = static_cast<uint32_t>(w[i]);
        if constexpr (sizeof(wchar_t) == 2) {
            if (cp >= 0xD800 && cp <= 0xDBFF) {
                if (i + 1 < w.size()) {
                    const uint32_t low = static_cast<uint32_t>(w[i + 1]);
                    if (low >= 0xDC00 && low <= 0xDFFF) {
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                        ++i;
                    } else {
                        cp = 0xFFFD;
                    }
                } else {
                    cp = 0xFFFD;
                }
            } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                cp = 0xFFFD;
            }
        }
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            cp = 0xFFFD;
        if (cp < 0x80) out.push_back(static_cast<char>(cp));
        else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}
#endif

std::filesystem::path log_file_path(const char *name) {
#if defined(_WIN32)
    // keep diagnostics in windows documents dir
    std::wstring wname;
    for (const char *p = name; *p; ++p)
        wname.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*p)));

    const wchar_t *profile = _wgetenv(L"USERPROFILE");
    std::filesystem::path base;
    if (profile && profile[0]) {
        base = std::filesystem::path(profile) / L"Documents";
    } else {
        const wchar_t *drive = _wgetenv(L"HOMEDRIVE");
        const wchar_t *home = _wgetenv(L"HOMEPATH");
        if (drive && drive[0] && home && home[0])
            base = std::filesystem::path(std::wstring(drive) + home) / L"Documents";
        else
            base = std::filesystem::path(L".");
    }
    const std::filesystem::path dir = base / L"SpaceRangersHD";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec || !std::filesystem::is_directory(dir, ec)) {
        const auto utf8dir = narrow(dir.wstring());
        std::fprintf(stderr, "MatrixGame: cannot create diagnostics directory '%s': %s\n",
                     utf8dir.c_str(), ec ? ec.message().c_str() : "not a directory");
    }
    return dir / wname;
#else
    // keep diagnostics separate from game cache files
    const char *xdg = std::getenv("XDG_DATA_HOME");
    const char *home = std::getenv("HOME");

    std::filesystem::path base;
    if (xdg && xdg[0] == '/') {
        base = std::filesystem::path(xdg);
    } else if (home && home[0] == '/') {
        base = std::filesystem::path(home) / ".local" / "share";
    } else {
        base = std::filesystem::path(".");
    }

    const std::filesystem::path dir = base / "SpaceRangersHD";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec || !std::filesystem::is_directory(dir, ec)) {
        std::fprintf(stderr, "MatrixGame: cannot create diagnostics directory '%s': %s\n",
                     dir.string().c_str(), ec ? ec.message().c_str() : "not a directory");
    }
    return dir / name;
#endif
}

std::string log_path(const char *name) {
    const auto path = log_file_path(name);
#if defined(_WIN32)
    return narrow(path.wstring());
#else
    return path.string();
#endif
}

FILE *open_log_file(const char *name) {
    const auto path = log_file_path(name);
#if defined(_WIN32)
    return _wfopen(path.c_str(), L"w");
#else
    return std::fopen(path.c_str(), "w");
#endif
}

void stamp(char *buf, size_t n) {
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char zone[16] = "UTC+00:00";
    char offset[8]{};
    if (std::strftime(offset, sizeof(offset), "%z", &tm) == 5) {
        std::snprintf(zone, sizeof(zone), "UTC%c%c%c:%c%c", offset[0], offset[1], offset[2], offset[3], offset[4]);
    }
    std::snprintf(buf, n, "%02d-%02d-%04d %02d:%02d:%02d %s", tm.tm_mday, tm.tm_mon + 1,
                  tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, zone);
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n\"'");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n\"'");
    return value.substr(first, last - first + 1);
}

std::string command_output(const char *command) {
#if defined(_WIN32)
    std::wstring wide_command;
    for (const unsigned char *p = reinterpret_cast<const unsigned char*>(command); *p; ++p)
        wide_command.push_back(static_cast<wchar_t>(*p));
    std::string result(16384, '\0');
    const int length = mgwin_command_output(wide_command.c_str(), result.data(), static_cast<uint32_t>(result.size()));
    if (length <= 0) return {};
    result.resize(static_cast<size_t>(length));
    return trim(result);
#else
    FILE *pipe = popen(command, "r");
    if (!pipe) return {};
    std::string result;
    char buffer[512];
    while (std::fgets(buffer, sizeof(buffer), pipe)) result += buffer;
    pclose(pipe);
    return trim(result);
#endif
}

std::string read_file_value(const std::string &path) {
    std::ifstream file(path);
    std::string value;
    if (file) std::getline(file, value);
    return trim(value);
}

#if defined(_WIN32)
struct WindowsProbe {
    std::string os = "Windows (version unavailable)";
    std::string cpu = "Unknown";
    unsigned cores = 0;
    unsigned long long ram_total = 0;
    unsigned long long ram_available = 0;
    std::string gpu = "Unknown";
    std::string driver = "Unknown";
    unsigned long long gpu_total = 0;
    unsigned long long gpu_available = 0;
    bool gpu_total_known = false;
    bool gpu_available_known = false;
};

const WindowsProbe &windows_probe() {
    static const WindowsProbe probe = [] {
        WindowsProbe result;
        const char *script =
            "powershell.exe -NoProfile -NonInteractive -Command \""
            "$ErrorActionPreference='SilentlyContinue'; "
            "$v=Get-ItemProperty 'HKLM:\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion'; "
            "$os=$v.ProductName; if ([int]$v.CurrentBuild -ge 22000) {$os=$os -replace 'Windows 10','Windows 11'}; "
            "if ($v.DisplayVersion) {$os += ' ' + $v.DisplayVersion}; "
            "$p=Get-WmiObject Win32_Processor | Select-Object -First 1; "
            "$m=Get-WmiObject Win32_OperatingSystem; "
            "$g=Get-WmiObject Win32_VideoController | Sort-Object AdapterRAM -Descending | Select-Object -First 1; "
            "$gt=0; $ga=0; $gk=0; "
            "$class='HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}'; "
            "$rg=Get-ChildItem $class -ErrorAction SilentlyContinue | Where-Object {$_.PSChildName -match '^\\d{4}$'} | ForEach-Object {Get-ItemProperty $_.PSPath} | Where-Object {$_.'HardwareInformation.AdapterString' -and ($_.'HardwareInformation.AdapterString' -like ('*'+$g.Name+'*'))} | Select-Object -First 1; "
            "if ($rg) {$physical=$rg.'HardwareInformation.qwMemorySize'; if (!$physical) {$physical=$rg.'HardwareInformation.MemorySize'}; if ($physical -gt 0) {$gt=[math]::Floor([double]$physical/1MB)}}; "
            "if ($gt -eq 0 -and $g.AdapterRAM -gt 0 -and $g.AdapterRAM -lt 4294967295) {$gt=[math]::Floor($g.AdapterRAM/1MB)}; "
            "try {$c=Get-WmiObject -Namespace root\\cimv2 -Class Win32_PerfFormattedData_GPUPerformanceCounters_GPUAdapterMemory -ErrorAction Stop; "
            "$best=$c | Sort-Object DedicatedLimit -Descending | Select-Object -First 1; "
            "if ($best -and $best.DedicatedLimit -gt 0) {$ga=[math]::Floor([math]::Max(0,$best.DedicatedLimit-$best.DedicatedUsage)/1MB); $gk=1}} catch {}; "
            "if (!$gk) {try {$c=Get-Counter '\\GPU Adapter Memory(*)\\Dedicated Limit','\\GPU Adapter Memory(*)\\Dedicated Usage' -ErrorAction Stop; "
            "$best=$c.CounterSamples | Where-Object {$_.Path -like '*Dedicated Limit'} | Sort-Object CookedValue -Descending | Select-Object -First 1; "
            "if ($best -and $best.CookedValue -gt 0) {$u=$c.CounterSamples | Where-Object {$_.Path -like '*Dedicated Usage' -and $_.InstanceName -eq $best.InstanceName} | Select-Object -First 1; "
            "if ($u) {$ga=[math]::Floor([math]::Max(0,$best.CookedValue-$u.CookedValue)/1MB); $gk=1}}} catch {}}; "
            "[Console]::WriteLine($os); [Console]::WriteLine($p.Name); "
            "[Console]::WriteLine(($p | Measure-Object -Property NumberOfLogicalProcessors -Sum).Sum); "
            "[Console]::WriteLine([math]::Floor($m.TotalVisibleMemorySize/1024)); [Console]::WriteLine([math]::Floor($m.FreePhysicalMemory/1024)); "
            "[Console]::WriteLine($g.Name); [Console]::WriteLine($g.DriverVersion); "
            "[Console]::WriteLine($gt); [Console]::WriteLine($ga); [Console]::WriteLine($gk)\"";
        const std::string data = command_output(script);
        std::istringstream in(data);
        std::string line;
        auto next = [&]() { std::string value; std::getline(in, value); return trim(value); };
        const std::string os = next(); if (!os.empty()) result.os = os;
        const std::string cpu = next(); if (!cpu.empty()) result.cpu = cpu;
        try { result.cores = static_cast<unsigned>(std::stoul(next())); } catch (...) {}
        try { result.ram_total = std::stoull(next()); } catch (...) {}
        try { result.ram_available = std::stoull(next()); } catch (...) {}
        const std::string gpu = next(); if (!gpu.empty()) result.gpu = gpu;
        const std::string driver = next(); if (!driver.empty()) result.driver = driver;
        try { result.gpu_total = std::stoull(next()); result.gpu_total_known = result.gpu_total > 0; } catch (...) {}
        try { result.gpu_available = std::stoull(next()); } catch (...) {}
        try { result.gpu_available_known = std::stoul(next()) != 0; } catch (...) {}
        return result;
    }();
    return probe;
}
#endif

std::string cpu_name() {
#if defined(_WIN32)
    return windows_probe().cpu;
#else
    std::ifstream file("/proc/cpuinfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("model name", 0) == 0) {
            const auto pos = line.find(':');
            if (pos != std::string::npos) return trim(line.substr(pos + 1));
        }
    }
    const auto fallback = command_output("lscpu 2>/dev/null | sed -n 's/^Model name:[[:space:]]*//p' | head -n 1");
    return fallback.empty() ? "Unknown" : fallback;
#endif
}

unsigned logical_cpu_count() {
#if defined(_WIN32)
    return windows_probe().cores;
#else
    const int count = SDL_GetCPUCount();
    return count > 0 ? static_cast<unsigned>(count) : 0;
#endif
}

std::string operating_system() {
#if defined(_WIN32)
    return windows_probe().os;
#else
    std::ifstream file("/etc/os-release");
    std::string line, id, name, version, pretty;
    while (std::getline(file, line)) {
        if (line.rfind("ID=", 0) == 0) id = trim(line.substr(3));
        else if (line.rfind("NAME=", 0) == 0) name = trim(line.substr(5));
        else if (line.rfind("VERSION_ID=", 0) == 0) version = trim(line.substr(11));
        else if (line.rfind("PRETTY_NAME=", 0) == 0) pretty = trim(line.substr(12));
    }
    if (pretty.empty()) pretty = name;
    std::string key = id;
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    static const char *rolling[] = {"arch", "cachyos", "endeavouros", "manjaro", "artix", "void", "gentoo", "opensuse-tumbleweed"};
    for (const char *item : rolling) if (key == item) return name.empty() ? key + " rolling" : trim(name) + " rolling";
    if (version.empty()) return pretty.empty() ? "Linux (unknown distribution)" : pretty;
    if (!name.empty()) return trim(name) + " " + version;
    return pretty;
#endif
}

void memory_info(unsigned long long &total_mb, unsigned long long &available_mb) {
#if defined(_WIN32)
    total_mb = windows_probe().ram_total;
    available_mb = windows_probe().ram_available;
#else
    std::ifstream file("/proc/meminfo");
    std::string key, unit;
    unsigned long long value = 0;
    while (file >> key >> value >> unit) {
        if (key == "MemTotal:") total_mb = value / 1024;
        else if (key == "MemAvailable:") available_mb = value / 1024;
    }
#endif
}

struct GpuInfo {
    std::string name = "Unknown";
    std::string driver = "Unknown";
    unsigned long long total_mb = 0;
    unsigned long long available_mb = 0;
    bool memory_known = false;
    bool memory_available_known = false;
};

GpuInfo gpu_info() {
    GpuInfo gpu;
#if defined(_WIN32)
    const auto &probe = windows_probe();
    gpu.name = probe.gpu;
    gpu.driver = probe.driver;
    gpu.total_mb = probe.gpu_total;
    gpu.available_mb = probe.gpu_available;
    gpu.memory_known = probe.gpu_total_known;
    gpu.memory_available_known = probe.gpu_available_known;
#else
    const auto nvidia = command_output("nvidia-smi --query-gpu=name,driver_version,memory.total,memory.free --format=csv,noheader,nounits 2>/dev/null");
    if (!nvidia.empty()) {
        std::istringstream in(nvidia);
        std::string total, free;
        std::getline(in, gpu.name, ',');
        std::getline(in, gpu.driver, ',');
        std::getline(in, total, ',');
        std::getline(in, free);
        try { gpu.total_mb = std::stoull(trim(total)); } catch (...) {}
        try { gpu.available_mb = std::stoull(trim(free)); } catch (...) {}
        gpu.memory_known = gpu.total_mb > 0;
        gpu.memory_available_known = gpu.memory_known;
    } else {
        gpu.name = command_output("lspci | grep -Ei 'VGA compatible controller|3D controller|Display controller' | head -n 1 | sed 's/^[^:]*: *//'");
        const auto gl_renderer = command_output("glxinfo -B 2>/dev/null | grep -m1 'OpenGL renderer string:' | sed 's/.*OpenGL renderer string: //'");
        if (gpu.name.empty()) gpu.name = gl_renderer.empty() ? "Unknown" : gl_renderer;
        const auto gl = command_output("glxinfo -B 2>/dev/null | grep -m1 'OpenGL version string:' | sed 's/.*OpenGL version string: //'");
        const auto driver_name = command_output("for f in /sys/class/drm/card*/device/driver; do [ -L \"$f\" ] && basename \"$(readlink \"$f\")\" && break; done");
        if (driver_name == "amdgpu" || driver_name == "i915" || driver_name == "xe" || gl.find("Mesa") != std::string::npos) {
            const auto pacman = command_output("pacman -Q mesa 2>/dev/null | awk '{print $2}'");
            const auto debian = command_output("dpkg-query -W -f='${Version}' mesa-vulkan-drivers 2>/dev/null");
            const auto fedora = command_output("rpm -q --qf '%{VERSION}-%{RELEASE}' mesa-dri-drivers 2>/dev/null");
            const auto mesa = !pacman.empty() ? pacman : !debian.empty() ? debian : fedora;
            gpu.driver = mesa.empty() ? (gl.empty() ? "Mesa (version unavailable)" : gl) : "Mesa " + mesa;
        } else if (!gl.empty()) {
            gpu.driver = gl;
        } else {
            const auto kernel_driver = command_output("for f in /sys/class/drm/card*/device/driver/module/version; do [ -r \"$f\" ] && cat \"$f\" && break; done");
            if (!kernel_driver.empty()) gpu.driver = kernel_driver;
        }
        std::error_code iter_ec;
        for (const auto &entry : std::filesystem::directory_iterator("/sys/class/drm", std::filesystem::directory_options::skip_permission_denied, iter_ec)) {
            const auto filename = entry.path().filename().string();
            if (filename.rfind("card", 0) != 0 || filename.find('-') != std::string::npos) continue;
            const auto device = entry.path() / "device";
            auto total = read_file_value((device / "mem_info_vram_total").string());
            auto used = read_file_value((device / "mem_info_vram_used").string());
            if (total.empty()) {
                total = read_file_value((device / "mem_info_gtt_total").string());
                used = read_file_value((device / "mem_info_gtt_used").string());
            }
            try {
                const auto total_bytes = std::stoull(total);
                const auto used_bytes = std::stoull(used);
                gpu.total_mb = total_bytes / (1024 * 1024);
                gpu.available_mb = (total_bytes > used_bytes ? total_bytes - used_bytes : 0) / (1024 * 1024);
                gpu.memory_known = gpu.total_mb > 0;
                gpu.memory_available_known = gpu.memory_known;
                break;
            } catch (...) {}
        }
    }
#endif
    if (gpu.driver != "Unknown" && gpu.driver.rfind("Mesa ", 0) != 0) {
        if (gpu.name.find("NVIDIA") != std::string::npos && gpu.driver.rfind("NVIDIA", 0) != 0)
            gpu.driver = "NVIDIA GeForce " + gpu.driver;
        else if (gpu.name.find("AMD") != std::string::npos && gpu.driver.rfind("AMD", 0) != 0)
            gpu.driver = "AMD Radeon " + gpu.driver;
        else if (gpu.name.find("Intel") != std::string::npos && gpu.driver.rfind("Intel", 0) != 0)
            gpu.driver = "Intel " + gpu.driver;
    }
    return gpu;
}

void refresh_gpu_memory(GpuInfo &gpu) {
#if defined(_WIN32)
    (void)gpu;
#else
    if (gpu.name.find("NVIDIA") != std::string::npos) {
        const auto value = command_output("nvidia-smi --query-gpu=memory.total,memory.free --format=csv,noheader,nounits 2>/dev/null");
        unsigned long long total = 0, available = 0;
        if (std::sscanf(value.c_str(), "%llu, %llu", &total, &available) == 2) {
            gpu.total_mb = total;
            gpu.available_mb = available;
            gpu.memory_known = true;
            gpu.memory_available_known = true;
        }
        return;
    }
    std::error_code ec;
    for (const auto &entry : std::filesystem::directory_iterator("/sys/class/drm", std::filesystem::directory_options::skip_permission_denied, ec)) {
        const auto filename = entry.path().filename().string();
        if (filename.rfind("card", 0) != 0 || filename.find('-') != std::string::npos) continue;
        const auto device = entry.path() / "device";
        auto total = read_file_value((device / "mem_info_vram_total").string());
        auto used = read_file_value((device / "mem_info_vram_used").string());
        if (total.empty()) {
            total = read_file_value((device / "mem_info_gtt_total").string());
            used = read_file_value((device / "mem_info_gtt_used").string());
        }
        try {
            const auto total_bytes = std::stoull(total);
            const auto used_bytes = std::stoull(used);
            gpu.total_mb = total_bytes / (1024 * 1024);
            gpu.available_mb = (total_bytes > used_bytes ? total_bytes - used_bytes : 0) / (1024 * 1024);
            gpu.memory_known = gpu.total_mb > 0;
            gpu.memory_available_known = gpu.memory_known;
            return;
        } catch (...) {}
    }
#endif
}

struct SystemInfo {
    std::string os;
    std::string cpu;
    GpuInfo gpu;
};

const SystemInfo &system_info() {
    static const SystemInfo info = [] {
        SystemInfo result;
        result.os = operating_system();
        result.cpu = cpu_name();
        const unsigned cores = logical_cpu_count();
        if (result.cpu.empty()) result.cpu = "Unknown";
        if (cores) result.cpu += " (" + std::to_string(cores) + " cores)";
        result.gpu = gpu_info();
        return result;
    }();
    return info;
}

std::string game_directory() {
#if defined(_WIN32)
    if (g_windows_module_base) {
        wchar_t module_path[32768]{};
        if (mgwin_module_path(reinterpret_cast<void*>(g_windows_module_base), module_path,
                              static_cast<uint32_t>(sizeof(module_path) / sizeof(module_path[0]))))
            return narrow(std::filesystem::path(module_path).parent_path().wstring());
    }
#else
    Dl_info module{};
    if (dladdr(reinterpret_cast<void*>(&MgLogInitWrite), &module) && module.dli_fname)
        return std::filesystem::absolute(module.dli_fname).parent_path().string();
#endif
    char *base = SDL_GetBasePath();
    if (base) {
        std::string path = trim(base);
        SDL_free(base);
        if (!path.empty()) return path;
    }
    std::error_code ec;
    return std::filesystem::current_path(ec).string();
}

std::string display_info() {
    SDL_DisplayMode mode{};
    if (SDL_GetCurrentDisplayMode(0, &mode) != 0 || mode.w <= 0 || mode.h <= 0 || mode.refresh_rate <= 0) return "Unknown";
    char value[96];
    std::snprintf(value, sizeof(value), "%dx%d %d Hz", mode.w, mode.h, mode.refresh_rate);
    return value;
}

std::string memory_value(unsigned long long value, bool known = true) {
    return known ? std::to_string(value) + " Mb" : "Unknown";
}

void write_stack(FILE *f) {
#if !defined(_WIN32)
    void *frames[64];
    int n = backtrace(frames, 64);
    char **syms = backtrace_symbols(frames, n);
    std::fprintf(f, "stack frames %d\n", n);
    if (syms) {
        for (int i = 0; i < n; ++i)
            std::fprintf(f, "  #%d %s\n", i, syms[i]);
        free(syms);
    }
#else
    std::fprintf(f, "stack: use host minidump / debugger on Windows build without backtrace\n");
#endif
}

}  // namespace

#if defined(_WIN32)
void MgLogSetWindowsVehInstalled(bool installed) noexcept {
    g_windows_veh_installed = installed;
}
void MgLogSetWindowsModuleBase(uintptr_t base) noexcept {
    g_windows_module_base = base;
}

void MgLogWindowsFirstChanceException(void *opaque) noexcept {
    // catch VEH access violations before host frame-based SEH handlers
    struct ExceptionRecordLayout {
        uint32_t code;
        uint32_t flags;
        void *nested;
        void *address;
        uint32_t parameter_count;
        uintptr_t information[15];
    };
    struct ExceptionPointersLayout { void *record; void *context; };
#if defined(_WIN64)
    // Windows x64 CONTEXT layout
    struct Context64Layout {
        uint64_t home[6];
        uint32_t context_flags;
        uint32_t mxcsr;
        uint16_t segments[6];
        uint32_t eflags;
        uint64_t debug_registers[6];
        uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
        uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
        uint64_t rip;
    };
    static_assert(offsetof(Context64Layout, rax) == 120, "unexpected Windows x64 CONTEXT layout");
    static_assert(offsetof(Context64Layout, rip) == 248, "unexpected Windows x64 CONTEXT layout");
#endif
    if (!opaque) return;
    const auto *pointers = static_cast<const ExceptionPointersLayout*>(opaque);
    if (!pointers->record) return;
    const auto *record = static_cast<const ExceptionRecordLayout*>(pointers->record);
    if (record->code != 0xC0000005u && record->code != 0xC0000006u) return;

    // cap first-chance records so handled probes do not flod log
    const long slot = __atomic_fetch_add(&g_veh_records, 1L, __ATOMIC_RELAXED);
    if (slot >= 8 || g_crash_path_w.empty()) return;
    if (__atomic_exchange_n(&g_veh_writer, 1L, __ATOMIC_ACQUIRE)) return;

    void *file = g_veh_crash_file;
    if (!file) {
        void *created = mgwin_create_file(g_crash_path_w.c_str(), 2u /* CREATE_ALWAYS */);
        if (!created || created == reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
            __atomic_store_n(&g_veh_writer, 0L, __ATOMIC_RELEASE);
            return;
        }
        void *expected = nullptr;
        if (!__atomic_compare_exchange_n(&g_veh_crash_file, &expected, created, false,
                                          __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
            mgwin_close(created);
            file = expected;
        } else {
            file = created;
        }
    }

    char line[4096];
    char *out = line;
    append_text(out, "FIRST-CHANCE WINDOWS EXCEPTION\r\n");
    append_text(out, "code="); append_hex(out, record->code); append_text(out, "\r\n");
    append_text(out, "exception_flags="); append_hex(out, record->flags); append_text(out, "\r\n");
    append_text(out, "exception_parameter_count="); append_hex(out, record->parameter_count); append_text(out, "\r\n");
    append_text(out, "exception_address="); append_hex(out, reinterpret_cast<uintptr_t>(record->address)); append_text(out, "\r\n");
    append_text(out, "matrixgame_module_base="); append_hex(out, g_windows_module_base); append_text(out, "\r\n");
    if ((record->code == 0xC0000005u || record->code == 0xC0000006u) && record->parameter_count >= 2) {
        append_text(out, "operation=");
        const uintptr_t op = record->information[0];
        append_text(out, op == 0 ? "read" : op == 1 ? "write" : op == 8 ? "execute" : "unknown");
        append_text(out, "\r\nfault_address="); append_hex(out, record->information[1]); append_text(out, "\r\n");
    }
#if defined(_WIN64)
    const auto *context = static_cast<const Context64Layout*>(pointers->context);
    if (context) {
        append_text(out, "context_arch=x86_64\r\ncontext_flags="); append_hex(out, context->context_flags);
        append_text(out, "\r\ncontext_rip="); append_hex(out, static_cast<uintptr_t>(context->rip));
        append_text(out, "\r\ncontext_rsp="); append_hex(out, static_cast<uintptr_t>(context->rsp));
        append_text(out, "\r\ncontext_rbp="); append_hex(out, static_cast<uintptr_t>(context->rbp));
        append_text(out, "\r\ncontext_eflags="); append_hex(out, context->eflags);
        append_text(out, "\r\ncontext_mxcsr="); append_hex(out, context->mxcsr);
        append_text(out, "\r\ncontext_seg_cs="); append_hex(out, context->segments[0]);
        append_text(out, "\r\ncontext_seg_ss="); append_hex(out, context->segments[5]);
        append_text(out, "\r\ncontext_rax="); append_hex(out, static_cast<uintptr_t>(context->rax));
        append_text(out, "\r\ncontext_rbx="); append_hex(out, static_cast<uintptr_t>(context->rbx));
        append_text(out, "\r\ncontext_rcx="); append_hex(out, static_cast<uintptr_t>(context->rcx));
        append_text(out, "\r\ncontext_rdx="); append_hex(out, static_cast<uintptr_t>(context->rdx));
        append_text(out, "\r\ncontext_rsi="); append_hex(out, static_cast<uintptr_t>(context->rsi));
        append_text(out, "\r\ncontext_rdi="); append_hex(out, static_cast<uintptr_t>(context->rdi));
        append_text(out, "\r\ncontext_r8="); append_hex(out, static_cast<uintptr_t>(context->r8));
        append_text(out, "\r\ncontext_r9="); append_hex(out, static_cast<uintptr_t>(context->r9));
        append_text(out, "\r\ncontext_r10="); append_hex(out, static_cast<uintptr_t>(context->r10));
        append_text(out, "\r\ncontext_r11="); append_hex(out, static_cast<uintptr_t>(context->r11));
        append_text(out, "\r\ncontext_r12="); append_hex(out, static_cast<uintptr_t>(context->r12));
        append_text(out, "\r\ncontext_r13="); append_hex(out, static_cast<uintptr_t>(context->r13));
        append_text(out, "\r\ncontext_r14="); append_hex(out, static_cast<uintptr_t>(context->r14));
        append_text(out, "\r\ncontext_r15="); append_hex(out, static_cast<uintptr_t>(context->r15));
        append_text(out, "\r\ncontext_dr0="); append_hex(out, static_cast<uintptr_t>(context->debug_registers[0]));
        append_text(out, "\r\ncontext_dr1="); append_hex(out, static_cast<uintptr_t>(context->debug_registers[1]));
        append_text(out, "\r\ncontext_dr2="); append_hex(out, static_cast<uintptr_t>(context->debug_registers[2]));
        append_text(out, "\r\ncontext_dr3="); append_hex(out, static_cast<uintptr_t>(context->debug_registers[3]));
        append_text(out, "\r\ncontext_dr6="); append_hex(out, static_cast<uintptr_t>(context->debug_registers[4]));
        append_text(out, "\r\ncontext_dr7="); append_hex(out, static_cast<uintptr_t>(context->debug_registers[5]));
        append_text(out, "\r\n");
    } else {
        append_text(out, "context=unavailable\r\n");
    }
#else
    append_text(out, "context_arch=unsupported (diagnostic register capture currently targets x86_64)\r\n");
#endif
    // handler stack below, not the faulting thread's saved stack
    void *frames[24]{};
    const unsigned short frame_count = RtlCaptureStackBackTrace(0, 24, frames, nullptr);
    append_text(out, "handler_stack_addresses=");
    for (unsigned short i = 0; i < frame_count; ++i) {
        if (i) *out++ = ',';
        append_hex(out, reinterpret_cast<uintptr_t>(frames[i]));
    }
    append_text(out, "\r\n\r\n");
    uint32_t written = 0;
    mgwin_write(file, line, static_cast<uint32_t>(out - line), &written);
    __atomic_store_n(&g_veh_writer, 0L, __ATOMIC_RELEASE);
}
#endif

std::string MgLogDirUtf8() {
    auto p = log_path("robots_init.log");
    auto slash = p.find_last_of("/\\");
    if (slash == std::string::npos)
        return ".";
    return p.substr(0, slash);
}

void MgLogEvent(const char *fmt, ...) {
    char body[kLine - 32];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);

    char ts[32];
    stamp(ts, sizeof(ts));

    std::lock_guard<std::mutex> lock(g_ring.mu);
    char *dst = g_ring.lines[g_ring.next];
    std::snprintf(dst, kLine, "%s | %s", ts, body);
    g_ring.next = (g_ring.next + 1) % kRing;
    if (g_ring.count < kRing)
        g_ring.count++;
}

void MgLogInitWrite(const char *extra) {
#if defined(_WIN32)
    g_crash_path_w = log_file_path("robots_crashlog.log").wstring();
    g_veh_records = 0;
    g_veh_writer = 0;
    if (g_veh_crash_file) { mgwin_close(g_veh_crash_file); g_veh_crash_file = nullptr; }
#endif
    const std::string path = log_path("robots_init.log");
    FILE *f = open_log_file("robots_init.log");
    if (!f) {
        std::fprintf(stderr, "MatrixGame: cannot open init log '%s': ", path.c_str());
        std::perror(nullptr);
        return;
    }

    char ts[64];
    stamp(ts, sizeof(ts));
    const auto &system = system_info();
    GpuInfo gpu = system.gpu;
    refresh_gpu_memory(gpu);
    unsigned long long ram_total = 0, ram_available = 0;
    memory_info(ram_total, ram_available);

    std::fprintf(f, "robots_init.log: %s\n", ts);
    std::fprintf(f, "Game dir: %s\n", game_directory().c_str());
    std::fprintf(f, "Logging dir: %s\n", MgLogDirUtf8().c_str());
    std::fprintf(f, "Operating system: %s\n", system.os.c_str());
    std::fprintf(f, "CPU: %s\n", system.cpu.c_str());
    std::fprintf(f, "GPU: %s\n", gpu.name.empty() ? "Unknown" : gpu.name.c_str());
    std::fprintf(f, "GPU driver: %s\n", gpu.driver.empty() ? "Unknown" : gpu.driver.c_str());
    std::fprintf(f, "RAM total: %s\n", memory_value(ram_total, ram_total > 0).c_str());
    std::fprintf(f, "RAM avail: %s\n", memory_value(ram_available, ram_available > 0).c_str());
    std::fprintf(f, "GPU memory total: %s\n", memory_value(gpu.total_mb, gpu.memory_known).c_str());
    std::fprintf(f, "GPU memory avail: %s\n", memory_value(gpu.available_mb, gpu.memory_available_known).c_str());
    std::fprintf(f, "Display size: %s\n", display_info().c_str());
    std::fprintf(f, "%s\n", g_MatrixMap ? "Stage objects... ok!" : "Stage objects... pending");

    if (g_MatrixMap) {
        std::fprintf(f, "Exit state: %d\n", g_ExitState);
        std::fprintf(f, "Map size: %d x %d\n", g_MatrixMap->m_Size.x, g_MatrixMap->m_Size.y);
        std::fprintf(f, "Move size: %d x %d\n", g_MatrixMap->m_SizeMove.x, g_MatrixMap->m_SizeMove.y);
        std::fprintf(f, "Zones: %d\n", g_MatrixMap->m_RN.m_ZoneCnt);
        std::fprintf(f, "Path budget: %d\n", g_MatrixMap->m_PathCalcBudget);
    } else {
        std::fprintf(f, "Exit state: %d\n", g_ExitState);
        std::fprintf(f, "Map size: not ready\nMove size: not ready\nZones: not ready\nPath budget: not ready\n");
    }

    if (extra && std::strstr(extra, "battle init done"))
        std::fprintf(f, "MatrixGame init done\n");
    else
        std::fprintf(f, "MatrixGame init starting\n");
    std::fprintf(f, "--- recent events (empty at init) ---\n");
    std::fclose(f);
    MgLogEvent("init log written");
}

void MgLogCrashWrite(const char *reason, bool include_map_state) {
#if defined(_WIN32)
    // append the unhandled summary to existing VEH record
    if (g_veh_crash_file) {
        static constexpr char prefix[] = "\r\nUNHANDLED EXCEPTION SUMMARY\r\nreason=";
        uint32_t written = 0;
        mgwin_write(g_veh_crash_file, prefix, static_cast<uint32_t>(sizeof(prefix) - 1), &written);
        const char *message = reason ? reason : "unknown";
        mgwin_write(g_veh_crash_file, message, static_cast<uint32_t>(std::strlen(message)), &written);
        static constexpr char newline[] = "\r\n";
        mgwin_write(g_veh_crash_file, newline, static_cast<uint32_t>(sizeof(newline) - 1), &written);
        return;
    }
#endif
    const std::string path = log_path("robots_crashlog.log");
    FILE *f = open_log_file("robots_crashlog.log");
    if (!f) {
        std::fprintf(stderr, "MatrixGame: cannot open crash log '%s': ", path.c_str());
        std::perror(nullptr);
        return;
    }

    char ts[32];
    stamp(ts, sizeof(ts));
    std::fprintf(f, "robots_crashlog.log %s\n", ts);
    std::fprintf(f, "reason %s\n", reason ? reason : "unknown");
    std::fprintf(f, "stage %s\n", g_mg_stage ? g_mg_stage : "?");
    std::fprintf(f, "exit_state %d\n", g_ExitState);
    std::fprintf(f, "log_dir %s\n", MgLogDirUtf8().c_str());

    if (include_map_state && g_MatrixMap) {
        std::fprintf(f, "map_size %d x %d move %d x %d zones %d\n", g_MatrixMap->m_Size.x, g_MatrixMap->m_Size.y,
                     g_MatrixMap->m_SizeMove.x, g_MatrixMap->m_SizeMove.y, g_MatrixMap->m_RN.m_ZoneCnt);
        std::fprintf(f, "path_budget %d\n", g_MatrixMap->m_PathCalcBudget);
        int robots = 0, cannons = 0;
        for (CMatrixMapStatic *o = CMatrixMapStatic::GetFirstLogic(); o; o = o->GetNextLogic()) {
            if (o->IsLiveRobot())
                robots++;
            else if (o->IsLiveCannon())
                cannons++;
        }
        std::fprintf(f, "live_robots %d live_cannons %d\n", robots, cannons);
    }

    std::fprintf(f, "--- stack ---\n");
    write_stack(f);

    if (include_map_state) {
        std::fprintf(f, "--- recent events ---\n");
        std::lock_guard<std::mutex> lock(g_ring.mu);
        int start = (g_ring.count < kRing) ? 0 : g_ring.next;
        for (int i = 0; i < g_ring.count; ++i) {
            int idx = (start + i) % kRing;
            std::fprintf(f, "%s\n", g_ring.lines[idx]);
        }
    } else {
        // avoid the event-ring mutex on early failure
        std::fprintf(f, "--- recent events omitted (minimal crash path) ---\n");
    }

    std::fclose(f);
}
