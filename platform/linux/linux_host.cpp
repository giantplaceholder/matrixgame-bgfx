// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "windows.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct FileSlot {
    int fd = -1;
    bool find = false;
    DIR* dir = nullptr;
    std::string pattern;
    std::string dir_path;
};

std::unordered_map<int, FileSlot> g_slots;
int g_next = 1;

std::string narrow(const wchar_t* text) {
    std::string out;
    if (!text) {
        return out;
    }
    for (; *text; ++text) {
        char ch = static_cast<char>(*text);
        out.push_back(ch == '\\' ? '/' : ch);
    }
    return out;
}

std::wstring widen(const char* text) {
    std::wstring out;
    if (!text) {
        return out;
    }
    for (; *text; ++text) {
        out.push_back(static_cast<wchar_t>(*text));
    }
    return out;
}

HANDLE alloc_slot(FileSlot slot) {
    int id = g_next++;
    g_slots.emplace(id, std::move(slot));
    return reinterpret_cast<HANDLE>(static_cast<intptr_t>(id));
}

FileSlot* slot_of(HANDLE handle) {
    auto it = g_slots.find(static_cast<int>(reinterpret_cast<intptr_t>(handle)));
    if (it == g_slots.end()) {
        return nullptr;
    }
    return &it->second;
}

bool match_simple(const std::string& name, const std::string& pattern) {
    if (pattern == "*" || pattern == "*.*") {
        return true;
    }
    if (pattern.size() >= 2 && pattern[0] == '*' && pattern[1] == '.') {
        auto dot = name.rfind('.');
        return dot != std::string::npos && name.substr(dot) == pattern.substr(1);
    }
    return name == pattern;
}

}  // namespace

HANDLE CreateFileA(const char* name, DWORD, DWORD, void*, DWORD disposition, DWORD, HANDLE) {
    if (!name) {
        return INVALID_HANDLE_VALUE;
    }
    std::string path = name;
    for (char& ch : path) {
        if (ch == '\\') {
            ch = '/';
        }
    }
    int flags = O_CLOEXEC;
    if (disposition == CREATE_ALWAYS) {
        flags |= O_CREAT | O_TRUNC | O_RDWR;
    } else {
        flags |= O_RDONLY;
    }
    int fd = open(path.c_str(), flags, 0644);
    if (fd < 0) {
        // Windows paths are case-insensitive. The game ships CFG/, the engine asks cfg/
        std::string alt = path;
        for (char& ch : alt) {
            if (ch >= 'a' && ch <= 'z') ch = char(ch - 32);
            else if (ch >= 'A' && ch <= 'Z') ch = char(ch + 32);
        }
        fd = open(alt.c_str(), flags, 0644);
    }
    if (fd < 0 && path.rfind("cfg/", 0) == 0) {
        std::string alt = "CFG/" + path.substr(4);
        fd = open(alt.c_str(), flags, 0644);
    }
    if (fd < 0) {
        return INVALID_HANDLE_VALUE;
    }
    FileSlot slot;
    slot.fd = fd;
    return alloc_slot(std::move(slot));
}

HANDLE CreateFileW(const wchar_t* name, DWORD access, DWORD share, void* security, DWORD disposition, DWORD flags, HANDLE template_file) {
    return CreateFileA(narrow(name).c_str(), access, share, security, disposition, flags, template_file);
}

BOOL ReadFile(HANDLE file, void* buffer, DWORD bytes, DWORD* read, void*) {
    FileSlot* slot = slot_of(file);
    if (!slot || slot->fd < 0) {
        return FALSE;
    }
    ssize_t n = ::read(slot->fd, buffer, bytes);
    if (n < 0) {
        return FALSE;
    }
    if (read) {
        *read = static_cast<DWORD>(n);
    }
    return TRUE;
}

BOOL WriteFile(HANDLE file, const void* buffer, DWORD bytes, DWORD* written, void*) {
    FileSlot* slot = slot_of(file);
    if (!slot || slot->fd < 0) {
        return FALSE;
    }
    ssize_t n = ::write(slot->fd, buffer, bytes);
    if (n < 0) {
        return FALSE;
    }
    if (written) {
        *written = static_cast<DWORD>(n);
    }
    return TRUE;
}

DWORD GetFileSize(HANDLE file, DWORD* high) {
    FileSlot* slot = slot_of(file);
    if (!slot || slot->fd < 0) {
        return 0xFFFFFFFFu;
    }
    struct stat st {};
    if (fstat(slot->fd, &st) != 0) {
        return 0xFFFFFFFFu;
    }
    if (high) {
        *high = static_cast<DWORD>(static_cast<unsigned long long>(st.st_size) >> 32);
    }
    return static_cast<DWORD>(st.st_size);
}

DWORD SetFilePointer(HANDLE file, LONG distance, LONG* high, DWORD method) {
    FileSlot* slot = slot_of(file);
    if (!slot || slot->fd < 0) {
        return 0xFFFFFFFFu;
    }
    int whence = SEEK_SET;
    if (method == FILE_CURRENT) {
        whence = SEEK_CUR;
    } else if (method == FILE_END) {
        whence = SEEK_END;
    }
    off_t pos = lseek(slot->fd, distance, whence);
    if (pos < 0) {
        return 0xFFFFFFFFu;
    }
    if (high) {
        *high = 0;
    }
    return static_cast<DWORD>(pos);
}

BOOL CloseHandle(HANDLE handle) {
    FileSlot* slot = slot_of(handle);
    if (!slot) {
        return FALSE;
    }
    if (slot->fd >= 0) {
        close(slot->fd);
    }
    if (slot->dir) {
        closedir(slot->dir);
    }
    g_slots.erase(static_cast<int>(reinterpret_cast<intptr_t>(handle)));
    return TRUE;
}

HANDLE FindFirstFileA(const char* name, WIN32_FIND_DATAA* data) {
    if (!name || !data) {
        return INVALID_HANDLE_VALUE;
    }
    std::string path = name;
    for (char& ch : path) {
        if (ch == '\\') {
            ch = '/';
        }
    }
    std::string dir = ".";
    std::string pattern = path;
    auto slash = path.find_last_of('/');
    if (slash != std::string::npos) {
        dir = path.substr(0, slash);
        pattern = path.substr(slash + 1);
    }
    if (pattern.find('*') == std::string::npos && pattern.find('?') == std::string::npos) {
        struct stat st {};
        if (stat(path.c_str(), &st) != 0) {
            std::string alt = path;
            if (alt.rfind("cfg/", 0) == 0) alt.replace(0, 4, "CFG/");
            else if (alt.rfind("CFG/", 0) == 0) alt.replace(0, 4, "cfg/");
            if (stat(alt.c_str(), &st) != 0) return INVALID_HANDLE_VALUE;
            path = alt;
        }
        std::memset(data, 0, sizeof(*data));
        data->dwFileAttributes = S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        std::strncpy(data->cFileName, pattern.c_str(), MAX_PATH - 1);
        FileSlot slot;
        slot.find = true;
        return alloc_slot(std::move(slot));
    }
    DIR* directory = opendir(dir.c_str());
    if (!directory) {
        return INVALID_HANDLE_VALUE;
    }
    FileSlot slot;
    slot.find = true;
    slot.dir = directory;
    slot.pattern = pattern;
    slot.dir_path = dir;
    HANDLE handle = alloc_slot(std::move(slot));
    if (!FindNextFileA(handle, data)) {
        FindClose(handle);
        return INVALID_HANDLE_VALUE;
    }
    return handle;
}

HANDLE FindFirstFileW(const wchar_t* name, WIN32_FIND_DATAW* data) {
    WIN32_FIND_DATAA narrow_data {};
    HANDLE handle = FindFirstFileA(narrow(name).c_str(), &narrow_data);
    if (handle == INVALID_HANDLE_VALUE || !data) {
        return handle;
    }
    std::memset(data, 0, sizeof(*data));
    data->dwFileAttributes = narrow_data.dwFileAttributes;
    std::wstring wide = widen(narrow_data.cFileName);
    std::wcsncpy(data->cFileName, wide.c_str(), MAX_PATH - 1);
    return handle;
}

BOOL FindNextFileA(HANDLE find, WIN32_FIND_DATAA* data) {
    FileSlot* slot = slot_of(find);
    if (!slot || !slot->dir || !data) {
        return FALSE;
    }
    while (dirent* entry = readdir(slot->dir)) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        if (!match_simple(name, slot->pattern)) {
            continue;
        }
        std::memset(data, 0, sizeof(*data));
        std::strncpy(data->cFileName, name.c_str(), MAX_PATH - 1);
        return TRUE;
    }
    return FALSE;
}

BOOL FindNextFileW(HANDLE find, WIN32_FIND_DATAW* data) {
    WIN32_FIND_DATAA narrow_data {};
    if (!FindNextFileA(find, &narrow_data) || !data) {
        return FALSE;
    }
    std::memset(data, 0, sizeof(*data));
    std::wstring wide = widen(narrow_data.cFileName);
    std::wcsncpy(data->cFileName, wide.c_str(), MAX_PATH - 1);
    return TRUE;
}

BOOL FindClose(HANDLE find) {
    return CloseHandle(find);
}

BOOL QueryPerformanceCounter(LARGE_INTEGER* counter) {
    timespec ts {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (counter) {
        counter->QuadPart = static_cast<long long>(ts.tv_sec) * 1000000000ll + ts.tv_nsec;
    }
    return TRUE;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER* frequency) {
    if (frequency) {
        frequency->QuadPart = 1000000000ll;
    }
    return TRUE;
}

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <cstdarg>
#include <cstdio>

// Win32 ShowCursor returns a show/hide *count*, negative once hidden. The engine
int mg_show_cursor(int show) {
    static int count = 1;
    count = show ? 1 : -1;
    if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) return count;
    const bool shown = SDL_ShowCursor(-1) == 1;
    if (shown != (count >= 0)) SDL_ShowCursor(count >= 0 ? 1 : 0);
    return count;
}

struct ShotCallback final : bgfx::CallbackI {
    void fatal(const char*, uint16_t, bgfx::Fatal::Enum, const char* str) override {
        std::fprintf(stderr, "bgfx fatal: %s\n", str ? str : "");
    }
    void traceVargs(const char*, uint16_t, const char* format, va_list args) override {
        char buf[400];
        std::vsnprintf(buf, sizeof(buf), format ? format : "", args);
        std::fprintf(stderr, "bgfx: %s\n", buf);
    }
    void profilerBegin(const char*, uint32_t, const char*, uint16_t) override {}
    void profilerBeginLiteral(const char*, uint32_t, const char*, uint16_t) override {}
    void profilerEnd() override {}
    uint32_t cacheReadSize(uint64_t) override { return 0; }
    bool cacheRead(uint64_t, void*, uint32_t) override { return false; }
    void cacheWrite(uint64_t, const void*, uint32_t) override {}
    void screenShot(const char* path, uint32_t w, uint32_t h, uint32_t pitch, bgfx::TextureFormat::Enum format, const void* data, uint32_t, bool yflip) override {
        extern void mgfx_write_shot(const char*, uint32_t, uint32_t, uint32_t, const void*, int, int);
        const int bgra = format != bgfx::TextureFormat::RGBA8;
        mgfx_write_shot(path, w, h, pitch, data, yflip ? 1 : 0, bgra);
    }
    void captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool) override {}
    void captureEnd() override {}
    void captureFrame(const void*, uint32_t) override {}
};
ShotCallback g_shot_callback;
#include <bx/platform.h>

#include <deque>
#include <iostream>
#include <string>
#include <vector>

namespace {
SDL_Window* g_window = nullptr;
bool g_bgfx_inited = false;
std::deque<MSG> g_messages;
std::wstring g_command_line = L"matrixgame";
std::vector<std::wstring> g_args;
std::vector<wchar_t*> g_arg_ptrs;

void push_key(UINT message, int vk) {
    MSG msg{};
    msg.message = message;
    msg.wParam = static_cast<WPARAM>(vk);
    g_messages.push_back(msg);
}

int sdl_to_vk(SDL_Keycode sym) {
    if (sym >= SDLK_a && sym <= SDLK_z) return int(sym - 32);
    if (sym >= SDLK_0 && sym <= SDLK_9) return int(sym);
    if (sym >= SDLK_F1 && sym <= SDLK_F12) return 0x70 + int(sym - SDLK_F1);
    switch (sym) {
        case SDLK_LEFT: return 0x25;
        case SDLK_UP: return 0x26;
        case SDLK_RIGHT: return 0x27;
        case SDLK_DOWN: return 0x28;
        case SDLK_ESCAPE: return 0x1B;
        case SDLK_RETURN: case SDLK_KP_ENTER: return 0x0D;
        case SDLK_SPACE: return 0x20;
        case SDLK_LSHIFT: case SDLK_RSHIFT: return 0x10;
        case SDLK_LCTRL: case SDLK_RCTRL: return 0x11;
        case SDLK_LALT: case SDLK_RALT: return 0x12;
        case SDLK_TAB: return 0x09;
        case SDLK_BACKSPACE: return 0x08;
        case SDLK_DELETE: return 0x2E;
        case SDLK_INSERT: return 0x2D;
        case SDLK_HOME: return 0x24;
        case SDLK_END: return 0x23;
        case SDLK_PAGEUP: return 0x21;
        case SDLK_PAGEDOWN: return 0x22;
        case SDLK_PAUSE: return 0x13;
        default: return 0;
    }
}

void pump_sdl() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        MSG msg{};
        msg.hwnd = g_window;
        if (event.type == SDL_QUIT) {
            msg.message = WM_CLOSE;
            g_messages.push_back(msg);
        } else if (event.type == SDL_MOUSEMOTION) {
            msg.message = WM_MOUSEMOVE;
            msg.lParam = (event.motion.y << 16) | (event.motion.x & 0xffff);
            g_messages.push_back(msg);
        } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            bool down = event.type == SDL_MOUSEBUTTONDOWN;
            bool dbl = down && event.button.clicks >= 2;
            msg.message = down ? WM_LBUTTONDOWN : WM_LBUTTONUP;
            if (dbl) msg.message = WM_LBUTTONDBLCLK;
            if (event.button.button == SDL_BUTTON_RIGHT) {
                msg.message = down ? (dbl ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN) : WM_RBUTTONUP;
            } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                msg.message = down ? WM_MBUTTONDOWN : WM_MBUTTONUP;
            }
            msg.lParam = (event.button.y << 16) | (event.button.x & 0xffff);
            g_messages.push_back(msg);
        } else if (event.type == SDL_MOUSEWHEEL) {
            msg.message = WM_MOUSEWHEEL;
            int delta = event.wheel.y * 120;
            msg.wParam = uintptr_t(uint32_t(int32_t(delta) << 16));
            int x = 0, y = 0;
            SDL_GetMouseState(&x, &y);
            msg.lParam = (y << 16) | (x & 0xffff);
            g_messages.push_back(msg);
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            int vk = sdl_to_vk(event.key.keysym.sym);
            if (vk) {
                msg.message = event.type == SDL_KEYDOWN ? WM_KEYDOWN : WM_KEYUP;
                msg.wParam = uintptr_t(vk);
                g_messages.push_back(msg);
            }
        }
    }
}

void bind_native_window(bgfx::Init& init, SDL_Window* window) {
    init.swapChain.ndt = nullptr;
    init.swapChain.nwh = nullptr;
    init.platformData.type = bgfx::NativeWindowHandleType::Default;
    if (!window) return;
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    const bool got = SDL_GetWindowWMInfo(window, &wmi) == SDL_TRUE;
    const char* drv = SDL_GetCurrentVideoDriver();
    std::cout << "sdl driver " << (drv ? drv : "?")
              << " wm " << int(got) << " sub " << (got ? int(wmi.subsystem) : -1)
              << " err " << SDL_GetError() << "\n";
    if (!got) return;
    if (wmi.subsystem == SDL_SYSWM_WAYLAND) {
        // bgfx treats Default as X11. Wayland needs the wl_display + wl_surface pair
        init.platformData.type = bgfx::NativeWindowHandleType::Wayland;
        init.swapChain.ndt = wmi.info.wl.display;
        init.swapChain.nwh = wmi.info.wl.surface;
    } else if (wmi.subsystem == SDL_SYSWM_X11) {
        init.swapChain.ndt = wmi.info.x11.display;
        init.swapChain.nwh = reinterpret_cast<void*>(static_cast<uintptr_t>(wmi.info.x11.window));
    }
    std::cout << "native type " << int(init.platformData.type)
              << " ndt " << init.swapChain.ndt
              << " nwh " << init.swapChain.nwh << std::endl;
}

SDL_Window* make_game_window(const char* title, int w, int h, Uint32 flags) {
    SDL_Window* win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
    if (!win && (flags & (SDL_WINDOW_VULKAN | SDL_WINDOW_OPENGL))) {
        std::cout << "SDL_CreateWindow flags " << flags << " failed: " << SDL_GetError() << ", retrying plain\n";
        win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    }
    return win;
}
}

void mg_set_command_line(int argc, char** argv) {
    g_args.clear();
    g_command_line.clear();
    for (int i = 0; i < argc; ++i) {
        std::wstring wide;
        for (const char* p = argv[i]; *p; ++p) {
            wide.push_back(static_cast<wchar_t>(*p));
        }
        if (i) g_command_line.push_back(L' ');
        g_command_line += wide;
        g_args.push_back(wide);
    }
}

intptr_t g_wndproc = 1;

ATOM RegisterClassExW(const WNDCLASSEXW* wc) {
    if (wc && wc->lpfnWndProc) g_wndproc = reinterpret_cast<intptr_t>(wc->lpfnWndProc);
    return 1;
}

HWND CreateWindowExA(DWORD, const char*, const char* title, DWORD style, int, int, int w, int h, HWND, void*, HINSTANCE, void*) {
    if (!g_window) {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        SDL_version sdlv{};
        SDL_GetVersion(&sdlv);
        std::cout << "matrixgame-bgfx sdl "
                  << int(sdlv.major) << "." << int(sdlv.minor) << "." << int(sdlv.patch)
                  << std::endl;
        std::string name = title ? title : "MatrixGame";
        int ww = w > 0 ? w : 1280;
        int hh = h > 0 ? h : 720;
        // borderless desktop when the host asked for the whole display
        SDL_DisplayMode desktop{};
        if (SDL_GetDesktopDisplayMode(0, &desktop) != 0) {
            desktop.w = desktop.h = 0;
        }
        bool fullscreen = desktop.w > 0 && ww >= desktop.w && hh >= desktop.h;
        if (const char* forced = std::getenv("MG_FULLSCREEN"))
            fullscreen = std::strcmp(forced, "0") != 0;
        (void)style;
        const Uint32 fs_flags = fullscreen ? Uint32(SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP) : 0u;
        if (fullscreen) {
            ww = desktop.w;
            hh = desktop.h;
        }
        bgfx::Init init;
        init.callback = &g_shot_callback;
        init.type = bgfx::RendererType::Vulkan;
        Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | fs_flags;
        if (const char* renderer = std::getenv("MG_RENDERER")) {
            if (std::strcmp(renderer, "gl") == 0) {
                init.type = bgfx::RendererType::OpenGL;
                flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | fs_flags;
            } else if (std::strcmp(renderer, "noop") == 0) {
                init.type = bgfx::RendererType::Noop;
                flags = SDL_WINDOW_SHOWN | fs_flags;
            }
        }
        std::cout << "renderer request " << int(init.type) << " fullscreen " << int(fullscreen)
                  << " env " << (std::getenv("MG_RENDERER") ? std::getenv("MG_RENDERER") : "null") << std::endl;
        g_window = make_game_window(name.c_str(), ww, hh, flags);
        if (!g_window && fullscreen) {
            std::cout << "fullscreen window failed, retrying windowed" << std::endl;
            g_window = make_game_window(name.c_str(), ww, hh, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
        }
        if (g_window) {
            SDL_ShowWindow(g_window);
            SDL_RaiseWindow(g_window);
            SDL_SetWindowInputFocus(g_window);
            // separate SDL window, show its cursor
            SDL_ShowCursor(1);
            SDL_SetCursor(nullptr);
        }
        init.limits.maxTransientVbSize = 64u << 20;
        init.limits.maxTransientIbSize = 16u << 20;
        init.limits.numDrawCalls = 65536;
        init.swapChain.width = uint32_t(ww);
        init.swapChain.height = uint32_t(hh);
        bind_native_window(init, g_window);
        bool ok = bgfx::init(init);
        g_bgfx_inited = ok;
        if (!ok && init.type == bgfx::RendererType::Vulkan) {
            // a failed init already deletes its context
            std::cout << "vulkan init failed, trying opengl" << std::endl;
            if (g_window) {
                SDL_DestroyWindow(g_window);
                g_window = nullptr;
            }
            g_window = make_game_window(name.c_str(), ww, hh, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | fs_flags);
            init.type = bgfx::RendererType::OpenGL;
            bind_native_window(init, g_window);
            ok = bgfx::init(init);
            g_bgfx_inited = ok;
        }
        std::cout << "MatrixGame window: "
                  << (ok ? bgfx::getRendererName(bgfx::getRendererType()) : "none")
                  << " ok " << ok << std::endl;
    }
    return g_window;
}

BOOL AdjustWindowRectEx(RECT*, DWORD, BOOL, DWORD) { return TRUE; }

// shut the device down, then the window
BOOL DestroyWindow(HWND) {
    extern void mgfx_shutdown_device();
    mgfx_shutdown_device();
    g_bgfx_inited = false;
    if (g_window) {
        SDL_SetWindowFullscreen(g_window, 0);
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    g_messages.clear();
    std::cout << "battle shutdown\n";
    return TRUE;
}

BOOL GetClientRect(HWND, RECT* rect) {
    int w = 1280, h = 720;
    if (g_window) SDL_GetWindowSize(g_window, &w, &h);
    if (rect) { rect->left = 0; rect->top = 0; rect->right = w; rect->bottom = h; }
    return TRUE;
}
intptr_t GetWindowLongPtr(HWND, int) { return g_wndproc; }
intptr_t SetWindowLongPtrW(HWND, int, intptr_t value) { intptr_t old = g_wndproc; g_wndproc = value ? value : 1; return old; }
BOOL PeekMessageW(MSG* msg, HWND, UINT, UINT, UINT) {
    pump_sdl();
    if (g_messages.empty() || !msg) return FALSE;
    *msg = g_messages.front();
    g_messages.pop_front();
    return TRUE;
}
LRESULT DispatchMessageW(const MSG* msg) {
    if (!msg) return 0;
    if (g_wndproc > 1) {
        auto proc = reinterpret_cast<WNDPROC>(g_wndproc);
        return proc(msg->hwnd ? msg->hwnd : g_window, msg->message, msg->wParam, msg->lParam);
    }
    return 0;
}
BOOL ShowWindow(HWND, int) { return TRUE; }
int MessageBoxA(HWND, const char* text, const char* title, UINT) {
    std::cerr << (title ? title : "Message") << ": " << (text ? text : "") << "\n";
    return 0;
}
BOOL ClipCursor(const RECT*) { return TRUE; }
void timeBeginPeriod(UINT) {}
void timeEndPeriod(UINT) {}
wchar_t* GetCommandLineW() { return g_command_line.data(); }
wchar_t** CommandLineToArgvW(const wchar_t*, int* count) {
    g_arg_ptrs.clear();
    for (auto& arg : g_args) g_arg_ptrs.push_back(arg.data());
    if (count) *count = static_cast<int>(g_arg_ptrs.size());
    return g_arg_ptrs.data();
}

