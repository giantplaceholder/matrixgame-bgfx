// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

// real SDL and Win32, the game reaches this through host_api.h

#include "host_api.h"
#include "mg_trace.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bx/platform.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <string>

void mgfx_reset_device();
void mgfx_shutdown_device();
void mgfx_write_shot(const char* path, uint32_t w, uint32_t h, uint32_t pitch, const void* data, int yflip, int bgra);

#if BX_PLATFORM_WINDOWS
#ifndef DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE ((HANDLE)-2)
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE ((HANDLE)-3)
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif
#endif

namespace {

constexpr uint32_t MSG_CLOSE = 0x10;
constexpr uint32_t MSG_KEYDOWN = 0x100;
constexpr uint32_t MSG_KEYUP = 0x101;
constexpr uint32_t MSG_MOUSEMOVE = 0x200;
constexpr uint32_t MSG_LBUTTONDOWN = 0x201;
constexpr uint32_t MSG_LBUTTONUP = 0x202;
constexpr uint32_t MSG_LBUTTONDBLCLK = 0x203;
constexpr uint32_t MSG_RBUTTONDOWN = 0x204;
constexpr uint32_t MSG_RBUTTONUP = 0x205;
constexpr uint32_t MSG_RBUTTONDBLCLK = 0x206;
constexpr uint32_t MSG_MBUTTONDOWN = 0x207;
constexpr uint32_t MSG_MBUTTONUP = 0x208;
constexpr uint32_t MSG_MOUSEWHEEL = 0x20a;

struct SdlMsg {
    uint32_t message = 0;
    uintptr_t wparam = 0;
    intptr_t lparam = 0;
};

SDL_Window* g_window = nullptr;
std::deque<SdlMsg> g_messages;
std::string g_renderer = "none";
int g_want_w = 0;
int g_want_h = 0;
int g_points_w = 0;
int g_points_h = 0;
int g_pixels_w = 0;
int g_pixels_h = 0;
int g_fit_tries = 0;
int g_borderless = 0;
int g_focus_tries = 0;
bool g_bgfx_ok = false;

#if BX_PLATFORM_WINDOWS
void enable_dpi_awareness() {
    // 1 SDL unit = 1 physical pixel. SDL_WINDOWS_DPI_SCALING would turn a
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "0");

    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (!user32) user32 = LoadLibraryA("user32.dll");
    using SetCtx = BOOL (WINAPI*)(HANDLE);
    auto set_ctx = user32 ? reinterpret_cast<SetCtx>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")) : nullptr;
    const char* how = "manifest";
    if (set_ctx) {
        if (set_ctx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) how = "permonitorv2";
        else if (set_ctx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE)) how = "permonitor";
        else if (set_ctx(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)) how = "system";
    } else if (user32) {
        using SetAware = BOOL (WINAPI*)();
        auto set_aware = reinterpret_cast<SetAware>(GetProcAddress(user32, "SetProcessDPIAware"));
        if (set_aware && set_aware()) how = "system-legacy";
    }
    if (std::strcmp(how, "manifest") == 0) {
        HMODULE shcore = LoadLibraryA("shcore.dll");
        using SetAw = HRESULT (WINAPI*)(int);
        auto set_aw = shcore ? reinterpret_cast<SetAw>(GetProcAddress(shcore, "SetProcessDpiAwareness")) : nullptr;
        if (set_aw && set_aw(2) == 0) how = "permonitor-shcore";
    }
    using GetCtx = HANDLE (WINAPI*)();
    auto get_ctx = user32 ? reinterpret_cast<GetCtx>(GetProcAddress(user32, "GetThreadDpiAwarenessContext")) : nullptr;
    void* ctx = get_ctx ? get_ctx() : nullptr;
    std::cout << "dpi awareness " << how << " ctx " << ctx << std::endl;
}

unsigned window_dpi(SDL_Window* window) {
    if (!window) return 96;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info) != SDL_TRUE || info.subsystem != SDL_SYSWM_WINDOWS) return 96;
    HMODULE user32 = GetModuleHandleA("user32.dll");
    using GetDpi = UINT (WINAPI*)(HWND);
    auto get_dpi = user32 ? reinterpret_cast<GetDpi>(GetProcAddress(user32, "GetDpiForWindow")) : nullptr;
    if (!get_dpi) return 96;
    UINT dpi = get_dpi(info.info.win.window);
    return dpi ? dpi : 96;
}
#else
void enable_dpi_awareness() {}
unsigned window_dpi(SDL_Window*) { return 96; }
#endif

void read_window_sizes() {
    g_points_w = g_want_w;
    g_points_h = g_want_h;
    g_pixels_w = g_want_w;
    g_pixels_h = g_want_h;
    if (!g_window) return;
    // hidden fullscreen window reports 0 the next battle
    if (g_borderless) return;
    int lw = 0, lh = 0, pw = 0, ph = 0;
    SDL_GetWindowSize(g_window, &lw, &lh);
    SDL_GetWindowSizeInPixels(g_window, &pw, &ph);
    if (lw > 0 && lh > 0) {
        g_points_w = lw;
        g_points_h = lh;
    }
    if (pw > 0 && ph > 0) {
        g_pixels_w = pw;
        g_pixels_h = ph;
    }
}

void to_game_pixels(int x, int y, int* ox, int* oy) {
    int gx = x;
    int gy = y;
    if (g_points_w > 0 && g_pixels_w > 0 && g_points_w != g_pixels_w)
        gx = int(std::lround(double(x) * double(g_pixels_w) / double(g_points_w)));
    if (g_points_h > 0 && g_pixels_h > 0 && g_points_h != g_pixels_h)
        gy = int(std::lround(double(y) * double(g_pixels_h) / double(g_points_h)));
    if (ox) *ox = gx;
    if (oy) *oy = gy;
}

void fit_client_pixels() {
    if (!g_window || g_want_w <= 0 || g_want_h <= 0) return;
    // desktop already picked the size
    if (g_borderless) {
        read_window_sizes();
        return;
    }
    read_window_sizes();
    if (g_pixels_w == g_want_w && g_pixels_h == g_want_h) {
        g_fit_tries = 0;
        return;
    }
    if (g_fit_tries >= 4) return;
    ++g_fit_tries;
    int set_w = g_want_w;
    int set_h = g_want_h;
    if (g_points_w > 0 && g_pixels_w > 0 && g_pixels_w != g_points_w) {
        set_w = int(std::lround(double(g_want_w) * double(g_points_w) / double(g_pixels_w)));
        set_h = int(std::lround(double(g_want_h) * double(g_points_h) / double(g_pixels_h)));
    }
    SDL_SetWindowSize(g_window, set_w, set_h);
    read_window_sizes();
    if ((g_pixels_w != g_want_w || g_pixels_h != g_want_h) && g_fit_tries >= 4) {
        std::cout << "dpi client stayed " << g_pixels_w << "x" << g_pixels_h
                  << " wanted " << g_want_w << "x" << g_want_h << std::endl;
    }
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
        const int bgra = format != bgfx::TextureFormat::RGBA8;
        mgfx_write_shot(path, w, h, pitch, data, yflip ? 1 : 0, bgra);
    }
    void captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool) override {}
    void captureEnd() override {}
    void captureFrame(const void*, uint32_t) override {}
};
ShotCallback g_shot_callback;

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

void push(uint32_t message, uintptr_t wparam, intptr_t lparam) {
    g_messages.push_back(SdlMsg{message, wparam, lparam});
}

void nudge_keyboard_focus() {
    if (!g_window || g_focus_tries >= 40)
        return;
    if (SDL_GetKeyboardFocus() == g_window) {
        g_focus_tries = 40;
        return;
    }
    // peek is called in a burst, don't burn the retries before the window is mapped
    static int pumps = 0;
    if ((++pumps % 15) != 1)
        return;
    ++g_focus_tries;
    SDL_RaiseWindow(g_window);
    SDL_SetWindowInputFocus(g_window);
}

void pump() {
    nudge_keyboard_focus();
    if (g_window) fit_client_pixels();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            push(MSG_CLOSE, 0, 0);
        } else if (event.type == SDL_WINDOWEVENT
                   && (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
                       || event.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED
                       || event.window.event == SDL_WINDOWEVENT_MOVED)) {
            fit_client_pixels();
        } else if (event.type == SDL_MOUSEMOTION) {
            int x = 0, y = 0;
            to_game_pixels(event.motion.x, event.motion.y, &x, &y);
            push(MSG_MOUSEMOVE, 0, (y << 16) | (x & 0xffff));
        } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            bool down = event.type == SDL_MOUSEBUTTONDOWN;
            bool dbl = down && event.button.clicks >= 2;
            uint32_t message = down ? MSG_LBUTTONDOWN : MSG_LBUTTONUP;
            uint32_t extra = 0;
            if (event.button.button == SDL_BUTTON_RIGHT) {
                message = down ? MSG_RBUTTONDOWN : MSG_RBUTTONUP;
                if (dbl) extra = MSG_RBUTTONDBLCLK;
            } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                message = down ? MSG_MBUTTONDOWN : MSG_MBUTTONUP;
            } else if (dbl) {
                extra = MSG_LBUTTONDBLCLK;
            }
            int x = 0, y = 0;
            to_game_pixels(event.button.x, event.button.y, &x, &y);
            intptr_t packed = (y << 16) | (x & 0xffff);
            push(message, 0, packed);
            // send the down too, fire listens for that
            if (extra) push(extra, 0, packed);
        } else if (event.type == SDL_MOUSEWHEEL) {
            int delta = event.wheel.y * 120;
            int x = 0, y = 0;
            SDL_GetMouseState(&x, &y);
            to_game_pixels(x, y, &x, &y);
            push(MSG_MOUSEWHEEL, uintptr_t(uint32_t(int32_t(delta) << 16)), (y << 16) | (x & 0xffff));
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            int vk = sdl_to_vk(event.key.keysym);
            if (vk) push(event.type == SDL_KEYDOWN ? MSG_KEYDOWN : MSG_KEYUP, uintptr_t(vk), 0);
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
              << " err " << SDL_GetError() << std::endl;
    if (!got) return;
    if (wmi.subsystem == SDL_SYSWM_WINDOWS) {
        init.swapChain.nwh = wmi.info.win.window;
    }
    std::cout << "native type " << int(init.platformData.type)
              << " ndt " << init.swapChain.ndt
              << " nwh " << init.swapChain.nwh << std::endl;
}


// bind MatrixGame window to primary monitor
static void primary_pos(int w, int h, int* x, int* y) {
    SDL_Rect b{};
    if (SDL_GetDisplayBounds(0, &b) != 0) {
        *x = SDL_WINDOWPOS_CENTERED;
        *y = SDL_WINDOWPOS_CENTERED;
        return;
    }
    *x = b.x + (b.w - w) / 2;
    *y = b.y + (b.h - h) / 2;
}

SDL_Window* make_window(const char* title, int w, int h, Uint32 flags) {
    // hidden until the first frame
    flags = (flags & ~SDL_WINDOW_SHOWN) | SDL_WINDOW_HIDDEN;
    int px = 0, py = 0;
    primary_pos(w, h, &px, &py);
    SDL_Window* win = SDL_CreateWindow(title, px, py, w, h, flags);
    if (win) {
        SDL_ShowCursor(SDL_DISABLE);
        primary_pos(w, h, &px, &py);
        SDL_SetWindowPosition(win, px, py);
    }
    mg_trace("[mg] SDL_CreateWindow flags=0x%x %dx%d drv=%s -> %p err=%s\n",
             flags, w, h, SDL_GetCurrentVideoDriver(), (void*)win, SDL_GetError());
    if (!win && (flags & SDL_WINDOW_OPENGL)) {
        std::cout << "SDL_CreateWindow flags " << flags << " failed: " << SDL_GetError() << ", retrying plain\n";
        const Uint32 plain = SDL_WINDOW_HIDDEN | (flags & (SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_FULLSCREEN));
        primary_pos(w, h, &px, &py);
        win = SDL_CreateWindow(title, px, py, w, h, plain);
        if (win) SDL_SetWindowPosition(win, px, py);
        mg_trace("[mg] SDL_CreateWindow retry plain -> %p err=%s\n", (void*)win, SDL_GetError());
    }
    return win;
}

bool init_bgfx(const char* title, int w, int h, bgfx::RendererType::Enum type, Uint32 flags) {
    g_want_w = w;
    g_want_h = h;
    g_fit_tries = 0;
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
        g_bgfx_ok = false;
    }
    g_window = make_window(title, w, h, flags);
    if (g_window) {
        SDL_SetWindowResizable(g_window, SDL_FALSE);
        fit_client_pixels();
    }
    read_window_sizes();
    const unsigned dpi = window_dpi(g_window);
    std::cout << "dpi " << dpi
              << " scale " << (double(dpi) / 96.0)
              << " request " << w << "x" << h
              << " points " << g_points_w << "x" << g_points_h
              << " pixels " << g_pixels_w << "x" << g_pixels_h << std::endl;
    bgfx::Init init;
    init.callback = &g_shot_callback;
    init.type = type;
    init.limits.maxTransientVbSize = 64u << 20;
    init.limits.maxTransientIbSize = 16u << 20;
    init.limits.numDrawCalls = 65536;
    init.swapChain.width = uint32_t(g_pixels_w > 0 ? g_pixels_w : w);
    init.swapChain.height = uint32_t(g_pixels_h > 0 ? g_pixels_h : h);
    bind_native_window(init, g_window);
    // a failed init already deletes its context
    const bool ok = bgfx::init(init);
    g_bgfx_ok = ok;
    if (ok) g_renderer = bgfx::getRendererName(bgfx::getRendererType());
    return ok;
}

}  // namespace

namespace {

struct RendererChoice {
    bgfx::RendererType::Enum type;
    Uint32 flags;
    const char* name;
};

// a backend without shader blobs draws nothing
bool choice_usable(const RendererChoice& choice) {
    if (choice.type == bgfx::RendererType::Noop) return true;
    return mgfx_shaders_ok() != 0;
}

}  // namespace

extern "C" int mg_sdl_open(const char* title, int width, int height, int popup) {
    if (g_window) return 1;
    enable_dpi_awareness();
#if BX_PLATFORM_WINDOWS
    // own window class, the host already registered SDL_app
    if (SDL_RegisterApp("MatrixGameBattle", CS_BYTEALIGNCLIENT | CS_OWNDC, NULL) != 0)
        mg_trace("[mg] SDL_RegisterApp failed: %s\n", SDL_GetError());
#endif
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_version sdlv{};
    SDL_GetVersion(&sdlv);
    std::cout << "matrixgame-bgfx sdl " << int(sdlv.major) << "." << int(sdlv.minor) << "." << int(sdlv.patch) << " v67" << std::endl;
    const char* name = title && title[0] ? title : "MatrixGame";
    int w = width > 0 ? width : 1280;
    int h = height > 0 ? height : 720;
    SDL_DisplayMode desktop{};
    if (SDL_GetDesktopDisplayMode(0, &desktop) != 0)
        desktop.w = desktop.h = 0;
    // popup means fullscreen even if the 2d size is under the desktop
    bool fullscreen = popup || (desktop.w > 0 && w >= desktop.w && h >= desktop.h);
    if (const char* forced = std::getenv("MG_FULLSCREEN"))
        fullscreen = std::strcmp(forced, "0") != 0;
    const int req_w = w;
    const int req_h = h;
    Uint32 fs_flags = 0;
    if (fullscreen) {
        fs_flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP;
        if (desktop.w > 0 && desktop.h > 0) {
            w = desktop.w;
            h = desktop.h;
        }
    }
    g_borderless = fullscreen ? 1 : 0;
    g_focus_tries = 0;

    // Vulkan only. OpenGL drops robot shadow volumes
    constexpr RendererChoice kDefaultOrder[] = {
        {bgfx::RendererType::Vulkan, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN, "vk"},
    };
    constexpr RendererChoice kNoopOnly[] = {
        {bgfx::RendererType::Noop, SDL_WINDOW_SHOWN, "noop"},
    };

    const char* env = std::getenv("MG_RENDERER");
    const RendererChoice* order = kDefaultOrder;
    size_t count = sizeof(kDefaultOrder) / sizeof(kDefaultOrder[0]);
    if (env && std::strcmp(env, "noop") == 0) {
        order = kNoopOnly;
        count = sizeof(kNoopOnly) / sizeof(kNoopOnly[0]);
    }
    std::cout << "renderer request " << order[0].name
              << " fullscreen " << g_borderless
              << " " << w << "x" << h
              << " env " << (env ? env : "vk") << std::endl;

    bool ok = false;
    for (size_t i = 0; i < count && !ok; ++i) {
        const RendererChoice& choice = order[i];
        if (i != 0) std::cout << order[i - 1].name << " unusable, trying " << choice.name << std::endl;
        if (!init_bgfx(name, w, h, choice.type, choice.flags | fs_flags)) {
            if (!fullscreen) continue;
            std::cout << "fullscreen window failed, retrying windowed" << std::endl;
            fs_flags = 0;
            g_borderless = 0;
            w = req_w;
            h = req_h;
            fullscreen = false;
            if (!init_bgfx(name, w, h, choice.type, choice.flags)) continue;
        }
        // Validate the backend bgfx returned, not the one we asked for
        if (choice_usable(choice)) {
            ok = true;
            break;
        }
        std::cout << "bgfx gave us " << g_renderer
                  << ", which has no shader blobs here" << std::endl;
        // Safe here: only a successful init may be shut down
        bgfx::shutdown();
    }
    std::cout << "MatrixGame window: " << (ok ? g_renderer : "none") << " ok " << ok << std::endl;
    mg_trace("[mg] mg_sdl_open exit: renderer=%s ok=%d window=%p\n",
             ok ? g_renderer.c_str() : "none", int(ok), (void*)g_window);
    return g_window ? 1 : 0;
}

extern "C" void mg_sdl_size(int* width, int* height) {
    read_window_sizes();
    const int w = g_pixels_w > 0 ? g_pixels_w : (g_want_w > 0 ? g_want_w : 1280);
    const int h = g_pixels_h > 0 ? g_pixels_h : (g_want_h > 0 ? g_want_h : 720);
    if (width) *width = w;
    if (height) *height = h;
}

// close the window and the device, leave sdl up
// quitting would reinit video next time and ChangeDisplaySettings on the host
extern "C" void mg_sdl_close(void) {
    mgfx_shutdown_device();
    g_bgfx_ok = false;
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    g_messages.clear();
    std::cout << "battle shutdown\n";
}

extern "C" int mg_sdl_poll(uint32_t* message, uintptr_t* wparam, intptr_t* lparam) {
    pump();
    if (g_messages.empty()) return 0;
    SdlMsg msg = g_messages.front();
    g_messages.pop_front();
    if (message) *message = msg.message;
    if (wparam) *wparam = msg.wparam;
    if (lparam) *lparam = msg.lparam;
    return 1;
}

extern "C" const char* mg_sdl_renderer_name(void) {
    return g_renderer.c_str();
}

extern "C" void mg_desktop_size(int* w, int* h) {
    int dw = 0, dh = 0;
    if (g_borderless && g_window)
        SDL_GetWindowSizeInPixels(g_window, &dw, &dh);
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

// Win32 ShowCursor returns a show/hide *count*, negative once hidden. The engine
int mg_show_cursor(int show) {
    static int count = -1;
    count = show ? 1 : -1;
    // os cursor stays off
    if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
        SDL_ShowCursor(SDL_DISABLE);
    return count;
}

extern "C" void mg_reveal_window(void) {
    if (!g_window) return;
    Uint32 flags = SDL_GetWindowFlags(g_window);
    if (flags & SDL_WINDOW_HIDDEN) {
        SDL_ShowWindow(g_window);
        SDL_RaiseWindow(g_window);
    }
    SDL_ShowCursor(SDL_DISABLE);
}
