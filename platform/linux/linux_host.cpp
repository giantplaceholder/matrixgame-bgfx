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
#include <dlfcn.h>

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
int g_borderless = 0;
char g_xkeys_prev[32] = {};
bool g_xkeys_live = false;
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

// physical key, not the letter the layout prints. russian йцукен still has to be wasd
int sdl_to_vk(SDL_Keysym key) {
    const SDL_Scancode sc = key.scancode;
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        return int('A' + (sc - SDL_SCANCODE_A));
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        return int('1' + (sc - SDL_SCANCODE_1));
    if (sc == SDL_SCANCODE_0)
        return int('0');
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12)
        return 0x70 + int(sc - SDL_SCANCODE_F1);
    switch (sc) {
        case SDL_SCANCODE_LEFT: return 0x25;
        case SDL_SCANCODE_UP: return 0x26;
        case SDL_SCANCODE_RIGHT: return 0x27;
        case SDL_SCANCODE_DOWN: return 0x28;
        case SDL_SCANCODE_ESCAPE: return 0x1B;
        case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: return 0x0D;
        case SDL_SCANCODE_SPACE: return 0x20;
        case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: return 0x10;
        case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return 0x11;
        case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: return 0x12;
        case SDL_SCANCODE_TAB: return 0x09;
        case SDL_SCANCODE_BACKSPACE: return 0x08;
        case SDL_SCANCODE_DELETE: return 0x2E;
        case SDL_SCANCODE_INSERT: return 0x2D;
        case SDL_SCANCODE_HOME: return 0x24;
        case SDL_SCANCODE_END: return 0x23;
        case SDL_SCANCODE_PAGEUP: return 0x21;
        case SDL_SCANCODE_PAGEDOWN: return 0x22;
        case SDL_SCANCODE_PAUSE: return 0x13;
        case SDL_SCANCODE_MINUS: return 0xBD;
        case SDL_SCANCODE_EQUALS: return 0xBB;
        case SDL_SCANCODE_LEFTBRACKET: return 0xDB;
        case SDL_SCANCODE_RIGHTBRACKET: return 0xDD;
        case SDL_SCANCODE_BACKSLASH: return 0xDC;
        case SDL_SCANCODE_GRAVE: return 0xC0;
        default: break;
    }
    const SDL_Keycode sym = key.sym;
    if (sym >= SDLK_a && sym <= SDLK_z) return int(sym - 32);
    if (sym >= SDLK_0 && sym <= SDLK_9) return int(sym);
    return 0;
}

// evdev code, the physical key. layout never enters into it
int linux_key_to_vk(int code) {
    if (code >= 16 && code <= 25) {
        static const int row[] = {'Q','W','E','R','T','Y','U','I','O','P'};
        return row[code - 16];
    }
    if (code >= 30 && code <= 38) {
        static const int row[] = {'A','S','D','F','G','H','J','K','L'};
        return row[code - 30];
    }
    if (code >= 44 && code <= 50) {
        static const int row[] = {'Z','X','C','V','B','N','M'};
        return row[code - 44];
    }
    if (code >= 2 && code <= 10)
        return '1' + (code - 2);
    if (code == 11)
        return '0';
    if (code >= 59 && code <= 68)
        return 0x70 + (code - 59);
    switch (code) {
        case 87: return 0x7A; // f11
        case 88: return 0x7B; // f12
        case 1: return 0x1B;
        case 28: case 96: return 0x0D;
        case 57: return 0x20;
        case 42: case 54: return 0x10;
        case 29: case 97: return 0x11;
        case 56: case 100: return 0x12;
        case 15: return 0x09;
        case 14: return 0x08;
        case 111: return 0x2E;
        case 110: return 0x2D;
        case 102: return 0x24;
        case 107: return 0x23;
        case 104: return 0x21;
        case 109: return 0x22;
        case 103: return 0x26;
        case 108: return 0x28;
        case 105: return 0x25;
        case 106: return 0x27;
        case 119: return 0x13;
        case 12: return 0xBD;
        case 13: return 0xBB;
        case 26: return 0xDB;
        case 27: return 0xDD;
        case 43: return 0xDC;
        case 41: return 0xC0;
        default: return 0;
    }
}

// the host window keeps x focus, so sdl never sees wasd. the keymap does not care who is focused
bool poll_x_keys() {
    if (!g_window)
        return false;
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (SDL_GetWindowWMInfo(g_window, &wmi) != SDL_TRUE)
        return false;
    if (wmi.subsystem != SDL_SYSWM_X11)
        return false;
    using XQueryFn = int (*)(void*, char*);
    using XLockFn = void (*)(void*);
    static void* lib = nullptr;
    static int tried = 0;
    if (!tried) {
        tried = 1;
        lib = dlopen("libX11.so.6", RTLD_LAZY | RTLD_NOLOAD);
        if (!lib)
            lib = dlopen("libX11.so.6", RTLD_LAZY);
    }
    if (!lib)
        return false;
    auto query = reinterpret_cast<XQueryFn>(dlsym(lib, "XQueryKeymap"));
    if (!query)
        return false;
    auto lock = reinterpret_cast<XLockFn>(dlsym(lib, "XLockDisplay"));
    auto unlock = reinterpret_cast<XLockFn>(dlsym(lib, "XUnlockDisplay"));
    char keys[32];
    std::memset(keys, 0, sizeof(keys));
    void* dpy = wmi.info.x11.display;
    if (lock) lock(dpy);
    query(dpy, keys);
    if (unlock) unlock(dpy);
    for (int code = 8; code < 256; ++code) {
        int bit = 1 << (code & 7);
        bool on = (keys[code >> 3] & bit) != 0;
        bool was = (g_xkeys_prev[code >> 3] & bit) != 0;
        if (on == was)
            continue;
        int vk = linux_key_to_vk(code - 8);
        if (!vk)
            continue;
        MSG msg{};
        msg.hwnd = g_window;
        msg.message = on ? WM_KEYDOWN : WM_KEYUP;
        msg.wParam = uintptr_t(vk);
        g_messages.push_back(msg);
    }
    std::memcpy(g_xkeys_prev, keys, sizeof(keys));
    g_xkeys_live = true;
    return true;
}

void pump_sdl() {
    poll_x_keys();
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
        } else if (!g_xkeys_live && (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)) {
            int vk = sdl_to_vk(event.key.keysym);
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
        const Uint32 plain = SDL_WINDOW_SHOWN | (flags & (SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_FULLSCREEN));
        win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, plain);
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

using XErrFn = int (*)(void*, void*);
using XSetErrFn = XErrFn (*)(XErrFn);
using XUnmapFn = int (*)(void*, unsigned long);
using XSyncFn = int (*)(void*, int);

void* xlib_handle() {
    static void* lib = nullptr;
    static int tried = 0;
    if (!tried) {
        tried = 1;
        lib = dlopen("libX11.so.6", RTLD_LAZY | RTLD_NOLOAD);
        if (!lib)
            lib = dlopen("libX11.so.6", RTLD_LAZY);
    }
    return lib;
}

template <typename T>
T xsym(const char* name) {
    void* lib = xlib_handle();
    return lib ? reinterpret_cast<T>(dlsym(lib, name)) : nullptr;
}

XErrFn g_host_xerr = nullptr;
int g_host_xerr_saved = 0;

int swallow_xerr(void*, void*) { return 0; }

// host already put in the x error handler
// save it before this sdl copy replaces it, put it back when the battle window goes
void save_host_xerr() {
    if (g_host_xerr_saved)
        return;
    auto set = xsym<XSetErrFn>("XSetErrorHandler");
    if (!set)
        return;
    g_host_xerr = set(swallow_xerr);
    set(g_host_xerr);
    g_host_xerr_saved = 1;
}

void use_battle_xerr() {
    auto set = xsym<XSetErrFn>("XSetErrorHandler");
    if (set)
        set(swallow_xerr);
}

void restore_host_xerr() {
    if (!g_host_xerr_saved)
        return;
    auto set = xsym<XSetErrFn>("XSetErrorHandler");
    if (set)
        set(g_host_xerr);
}

// unmap first
// destroy on a mapped fullscreen window tells the wm to resize the host, they share this x screen
void unmap_battle_window(SDL_Window* window) {
    if (!window)
        return;
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (SDL_GetWindowWMInfo(window, &wmi) != SDL_TRUE)
        return;
    if (wmi.subsystem != SDL_SYSWM_X11)
        return;
    auto unmap = xsym<XUnmapFn>("XUnmapWindow");
    auto sync = xsym<XSyncFn>("XSync");
    if (!unmap)
        return;
    unmap(wmi.info.x11.display, static_cast<unsigned long>(wmi.info.x11.window));
    // round trip so sdl sees the window as unmapped
    if (sync)
        sync(wmi.info.x11.display, 0);
}

HWND CreateWindowExA(DWORD, const char*, const char* title, DWORD style, int, int, int w, int h, HWND, void*, HINSTANCE, void*) {
    if (!g_window) {
        save_host_xerr();
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        use_battle_xerr();
        SDL_version sdlv{};
        SDL_GetVersion(&sdlv);
        std::cout << "matrixgame-bgfx sdl "
                  << int(sdlv.major) << "." << int(sdlv.minor) << "." << int(sdlv.patch)
                  << " v48" << std::endl;
        std::string name = title ? title : "MatrixGame";
        int ww = w > 0 ? w : 1280;
        int hh = h > 0 ? h : 720;
        SDL_DisplayMode desktop{};
        if (SDL_GetDesktopDisplayMode(0, &desktop) != 0) {
            desktop.w = desktop.h = 0;
        }
        // popup means fullscreen even if the 2d size is under the desktop
        bool fullscreen = (style & WS_POPUP) != 0
            || (desktop.w > 0 && ww >= desktop.w && hh >= desktop.h);
        if (const char* forced = std::getenv("MG_FULLSCREEN"))
            fullscreen = std::strcmp(forced, "0") != 0;
        const int req_w = ww;
        const int req_h = hh;
        Uint32 fs_flags = 0;
        if (fullscreen) {
            fs_flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP;
            if (desktop.w > 0 && desktop.h > 0) {
                ww = desktop.w;
                hh = desktop.h;
            }
        }
        g_borderless = fullscreen ? 1 : 0;
        std::memset(g_xkeys_prev, 0, sizeof(g_xkeys_prev));
        g_xkeys_live = false;
        bgfx::Init init;
        init.callback = &g_shot_callback;
        init.type = bgfx::RendererType::Vulkan;
        Uint32 flags = SDL_WINDOW_SHOWN | fs_flags;
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
            fs_flags = 0;
            g_borderless = 0;
            ww = req_w;
            hh = req_h;
            flags = SDL_WINDOW_SHOWN;
            g_window = make_game_window(name.c_str(), ww, hh, flags);
        }
        if (g_window) {
            SDL_ShowWindow(g_window);
            SDL_RaiseWindow(g_window);
            SDL_SetWindowInputFocus(g_window);
            int aw = 0, ah = 0;
            SDL_GetWindowSize(g_window, &aw, &ah);
            if (aw > 0 && ah > 0) {
                ww = aw;
                hh = ah;
            }
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

// device first, then the window
// leave sdl up, quit closes our display, drops libx11 and the vulkan loader, and swaps the host x handler
BOOL DestroyWindow(HWND) {
    extern void mgfx_shutdown_device();
    mgfx_shutdown_device();
    g_bgfx_inited = false;
    if (g_window) {
        unmap_battle_window(g_window);
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    std::memset(g_xkeys_prev, 0, sizeof(g_xkeys_prev));
    g_xkeys_live = false;
    restore_host_xerr();
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

extern "C" void mg_desktop_size(int* w, int* h) {
    int dw = 0, dh = 0;
    if (g_borderless && g_window)
        SDL_GetWindowSize(g_window, &dw, &dh);
    if (dw <= 0 || dh <= 0) {
        SDL_DisplayMode mode{};
        if (SDL_WasInit(SDL_INIT_VIDEO) && SDL_GetDesktopDisplayMode(0, &mode) == 0) {
            dw = mode.w;
            dh = mode.h;
        }
    }
    if (dw <= 0 || dh <= 0) {
        dw = 1280;
        dh = 720;
    }
    if (w) *w = dw;
    if (h) *h = dh;
}

