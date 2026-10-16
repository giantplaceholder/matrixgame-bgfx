// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "d3d9.h"
#include "shader_bins.h"
#include "shader_mod.h"
#include "shader_atest.h"

#include <bgfx/bgfx.h>
// blobs are GLSL, SPIR-V and DXBC, no DXIL
#ifdef BGFX_PLATFORM_SUPPORTS_DXIL
#undef BGFX_PLATFORM_SUPPORTS_DXIL
#endif
#define BGFX_PLATFORM_SUPPORTS_DXIL 0
#include <bgfx/embedded_shader.h>
#include <bimg/decode.h>
#include <png.h>
#include <bx/allocator.h>
#include <bx/error.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif
extern int g_ScreenX, g_ScreenY;

static std::vector<IDirect3DTexture9*> g_live_tex;

void mgfx_track_texture(IDirect3DTexture9* tex, int alive) {
    if (!tex) return;
    if (alive) {
        g_live_tex.push_back(tex);
        return;
    }
    for (size_t i = 0; i < g_live_tex.size(); ++i) {
        if (g_live_tex[i] == tex) {
            g_live_tex[i] = g_live_tex.back();
            g_live_tex.pop_back();
            return;
        }
    }
}

static std::string wide_to_utf8(const wchar_t* w) {
    std::string out;
    if (!w) return out;
    for (const wchar_t* p = w; *p; ++p) {
        uint32_t cp = static_cast<uint32_t>(*p);
        if (sizeof(wchar_t) == 2 && cp >= 0xD800 && cp <= 0xDBFF && p[1]) {
            uint32_t lo = static_cast<uint32_t>(p[1]);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
                ++p;
            }
        }
        if (cp < 0x80) out.push_back(char(cp));
        else if (cp < 0x800) {
            out.push_back(char(0xC0 | (cp >> 6)));
            out.push_back(char(0x80 | (cp & 0x3F)));
        }
        else if (cp < 0x10000) {
            out.push_back(char(0xE0 | (cp >> 12)));
            out.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(char(0x80 | (cp & 0x3F)));
        }
        else {
            out.push_back(char(0xF0 | (cp >> 18)));
            out.push_back(char(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(char(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

static void mkdir_one(const char* path) {
#if defined(_WIN32)
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

static void mkdir_parents(const std::string& file) {
    std::string dir = file;
    auto slash = dir.find_last_of('/');
    if (slash == std::string::npos) return;
    dir.resize(slash);
    for (size_t i = 1; i <= dir.size(); ++i) {
        if (i == dir.size() || dir[i] == '/') {
            char saved = 0;
            if (i < dir.size()) {
                saved = dir[i];
                dir[i] = 0;
            }
            if (dir != "." && dir != "/") mkdir_one(dir.c_str());
            if (i < dir.size()) dir[i] = saved;
        }
    }
}

int mg_create_directory(const wchar_t* path) {
    if (!path || !path[0]) return 0;
    std::string u = wide_to_utf8(path);
    for (char& c : u) if (c == '\\') c = '/';
    if (!u.empty() && u.back() != '/') u.push_back('/');
    u.push_back('x');
    mkdir_parents(u);
    return 1;
}

void mgfx_write_shot(const char* path, uint32_t w, uint32_t h, uint32_t pitch, const void* data, int yflip, int bgra) {
    if (!data || !w || !h) return;
    std::string name = (path && path[0]) ? path : "mg-shot.ppm";
    for (char& c : name) if (c == '\\') c = '/';
    mkdir_parents(name);
    if (!pitch) pitch = w * 4;
    const bool png = name.size() >= 4 && (name.compare(name.size() - 4, 4, ".png") == 0 || name.compare(name.size() - 4, 4, ".PNG") == 0);
    const auto* src = static_cast<const uint8_t*>(data);
    auto put = [&](FILE* out, const uint8_t* p) {
        const uint8_t r = bgra ? p[2] : p[0];
        const uint8_t b = bgra ? p[0] : p[2];
        std::fputc(r, out);
        std::fputc(p[1], out);
        std::fputc(b, out);
    };
    if (!png) {
        FILE* out = std::fopen(name.c_str(), "wb");
        if (!out) return;
        std::fprintf(out, "P6\n%u %u\n255\n", w, h);
        for (uint32_t y = 0; y < h; ++y) {
            uint32_t row = yflip ? (h - 1 - y) : y;
            const uint8_t* p = src + row * pitch;
            for (uint32_t x = 0; x < w; ++x) {
                put(out, p);
                p += 4;
            }
        }
        std::fclose(out);
        std::fprintf(stderr, "screenshot %s %ux%u\n", name.c_str(), w, h);
        return;
    }
    std::vector<uint8_t> rgb(size_t(w) * h * 3);
    for (uint32_t y = 0; y < h; ++y) {
        uint32_t row = yflip ? (h - 1 - y) : y;
        const uint8_t* p = src + size_t(row) * pitch;
        uint8_t* d = rgb.data() + size_t(y) * w * 3;
        for (uint32_t x = 0; x < w; ++x) {
            d[0] = bgra ? p[2] : p[0];
            d[1] = p[1];
            d[2] = bgra ? p[0] : p[2];
            p += 4;
            d += 3;
        }
    }
    png_image image;
    std::memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    image.width = w;
    image.height = h;
    image.format = PNG_FORMAT_RGB;
    if (png_image_write_to_file(&image, name.c_str(), 0, rgb.data(), int(w * 3), nullptr) == 0)
        std::fprintf(stderr, "screenshot png fail %s\n", image.message);
    else
        std::fprintf(stderr, "screenshot %s %ux%u\n", name.c_str(), w, h);
}

static std::string g_shot_path;

void mgfx_request_screenshot(const wchar_t* path) {
    g_shot_path = wide_to_utf8(path);
    for (char& c : g_shot_path) if (c == '\\') c = '/';
    if (g_shot_path.empty()) g_shot_path = "screenshot.png";
    mkdir_parents(g_shot_path);
    if (!bgfx::getCaps()) return;
    bgfx::requestScreenShot(BGFX_INVALID_HANDLE, g_shot_path.c_str());
}

static void invalidate_live_textures() {
    for (auto* t : g_live_tex) {
        if (!t) continue;
        t->bgfx_tex = 0xFFFF;
        t->fb = 0xFFFF;
        t->rt_view = -1;
        t->rt_frame = -1;
    }
}
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

constexpr uint16_t VIEW_WORLD = 16;
constexpr uint16_t VIEW_UI = 17;

IDirect3DSurface9* g_rt = nullptr;
IDirect3DSurface9* g_cpu_target = nullptr;
void mgfx_begin_cpu_target(IDirect3DSurface9* surface) { g_cpu_target = surface; }
void mgfx_end_cpu_target() { g_cpu_target = nullptr; }
uint16_t g_draw_view = VIEW_WORLD;
int g_target_w = 0;
int g_target_h = 0;
uint32_t g_clear_rgba = 0x1a2430ff;
bool g_clear_color = false;

struct Viewport {
    uint32_t x = 0, y = 0, w = 0, h = 0;
    float minz = 0, maxz = 1;
};
Viewport g_vp;

namespace {
bx::DefaultAllocator g_alloc;
bgfx::ProgramHandle g_prog_tex = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle g_prog_col = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle g_prog_mod = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle g_prog_tex_at = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle g_prog_mod_at = BGFX_INVALID_HANDLE;
bgfx::UniformHandle g_sampler = BGFX_INVALID_HANDLE;
bgfx::UniformHandle g_sampler_mask = BGFX_INVALID_HANDLE;
bgfx::UniformHandle g_mvp = BGFX_INVALID_HANDLE;
bgfx::TextureHandle g_white = BGFX_INVALID_HANDLE;
bgfx::VertexLayout g_layout;
bgfx::VertexLayout g_layout_col;
bgfx::VertexLayout g_layout_mod;
bool g_ready = false;
uint32_t g_fvf = 0;
uint32_t g_stride = 0;
IDirect3DVertexBuffer9* g_vb = nullptr;
uint32_t g_vb_offset = 0;
IDirect3DIndexBuffer9* g_ib = nullptr;
IDirect3DTexture9* g_texture = nullptr;
IDirect3DTexture9* g_stages[8] = {};
uint32_t g_stage_gen[8] = {};
uint32_t g_tex_serial = 1;
uint32_t g_fog = 0;
uint32_t g_fog_color = 0xFF87A0B8u;
float g_fog_start = 2000.f;
float g_fog_end = 2800.f;
uint32_t g_draws = 0;
uint32_t g_tex_draws = 0;
uint32_t g_frames = 0;
float g_world[16];
float g_view[16];
float g_proj[16];
uint32_t g_state_alpha = 0;
uint32_t g_state_atest = 0;
uint32_t g_alpha_ref = 0x08;
uint32_t g_alpha_func = D3DCMP_GREATEREQUAL;
uint32_t g_state_z = 1;
uint32_t g_state_zwrite = 1;
uint32_t g_cull = 3;
uint32_t g_blend_src = D3DBLEND_SRCALPHA;
uint32_t g_blend_dst = D3DBLEND_INVSRCALPHA;
uint32_t g_color_write = 0xF;
uint32_t g_stencil = 0;
uint32_t g_stencil_func = D3DCMP_ALWAYS;
uint32_t g_stencil_fail = D3DSTENCILOP_KEEP;
uint32_t g_stencil_zfail = D3DSTENCILOP_KEEP;
uint32_t g_stencil_pass = D3DSTENCILOP_KEEP;
uint32_t g_stencil_ref = 0;
uint32_t g_stencil_mask = 0xffffffffu;
uint32_t g_stencil_wmask = 0xffffffffu;
uint32_t g_skipped = 0;
uint32_t g_dropped = 0;
uint32_t g_tfactor = 0xFFFFFFFFu;
uint32_t g_color_op = 4;      // D3DTOP_MODULATE
uint32_t g_color_arg1 = 2;    // D3DTA_TEXTURE
uint32_t g_color_arg2 = 1;    // D3DTA_CURRENT
uint32_t g_color_op1 = 1;     // stage 1, D3DTOP_DISABLE
uint32_t g_color_op2 = 1;     // stage 2, D3DTOP_DISABLE
uint32_t g_color_arg1_2 = 2;  // stage 2 arg1
uint32_t g_color_arg2_2 = 1;  // stage 2 arg2
uint32_t g_op2_gen = 0;       // g_tex_serial when stage-2 COLOROP was written
uint32_t g_tex_index0 = 0;    // D3DTSS_TEXCOORDINDEX for stage 0
int g_tint_tfactor = 0;       // multiply baked rgb by TEXTUREFACTOR (gloss albedo)
uint32_t g_alpha_op = 2;      // D3DTOP_SELECTARG1
uint32_t g_alpha_arg1 = 2;    // D3DTA_TEXTURE
uint32_t g_alpha_arg2 = 1;    // D3DTA_CURRENT
uint32_t g_mag_filter = 2;    // D3DTEXF_LINEAR
uint32_t g_addr_u = 1;        // WRAP
uint32_t g_addr_v = 1;
int g_water_draw = 0;
static IDirect3DTexture9* g_mask_tex = nullptr;

void identity(float* m) {
    std::memset(m, 0, sizeof(float) * 16);
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

void transpose(const float* in, float* out) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            out[c * 4 + r] = in[r * 4 + c];
        }
    }
}

const bgfx::EmbeddedShader kTexVs[] = {
    BGFX_EMBEDDED_SHADER(vs_tex),
    BGFX_EMBEDDED_SHADER_END()
};
const bgfx::EmbeddedShader kTexFs[] = {
    BGFX_EMBEDDED_SHADER(fs_tex),
    BGFX_EMBEDDED_SHADER_END()
};
const bgfx::EmbeddedShader kColVs[] = {
    BGFX_EMBEDDED_SHADER(vs_col),
    BGFX_EMBEDDED_SHADER_END()
};
const bgfx::EmbeddedShader kColFs[] = {
    BGFX_EMBEDDED_SHADER(fs_col),
    BGFX_EMBEDDED_SHADER_END()
};

const bgfx::EmbeddedShader kModVs[] = {
    BGFX_EMBEDDED_SHADER(vs_mod),
    BGFX_EMBEDDED_SHADER_END()
};
const bgfx::EmbeddedShader kModFs[] = {
    BGFX_EMBEDDED_SHADER(fs_mod),
    BGFX_EMBEDDED_SHADER_END()
};
const bgfx::EmbeddedShader kTexAtFs[] = {
    BGFX_EMBEDDED_SHADER(fs_tex_at),
    BGFX_EMBEDDED_SHADER_END()
};
const bgfx::EmbeddedShader kModAtFs[] = {
    BGFX_EMBEDDED_SHADER(fs_mod_at),
    BGFX_EMBEDDED_SHADER_END()
};

void ensure_views() {
    uint16_t w = uint16_t(g_ScreenX > 0 ? g_ScreenX : 1024);
    uint16_t h = uint16_t(g_ScreenY > 0 ? g_ScreenY : 768);
    bgfx::setViewMode(VIEW_WORLD, bgfx::ViewMode::Sequential);
    bgfx::setViewMode(VIEW_UI, bgfx::ViewMode::Sequential);
    bgfx::setViewRect(VIEW_WORLD, 0, 0, w, h);
    bgfx::setViewRect(VIEW_UI, 0, 0, w, h);
    bgfx::setViewClear(VIEW_UI, BGFX_CLEAR_DEPTH, 0, 1.f, 0);
    bgfx::setViewFrameBuffer(VIEW_WORLD, BGFX_INVALID_HANDLE);
    bgfx::setViewFrameBuffer(VIEW_UI, BGFX_INVALID_HANDLE);
}

// True if the embedded shader blobs cover the renderer bgfx actually selected
extern "C" int mgfx_shaders_ok(void) {
    const auto type = bgfx::getRendererType();
    const auto vs = bgfx::createEmbeddedShader(kTexVs, type, "vs_tex");
    const auto fs = bgfx::createEmbeddedShader(kTexFs, type, "fs_tex");
    const int ok = bgfx::isValid(vs) && bgfx::isValid(fs);
    if (bgfx::isValid(vs)) bgfx::destroy(vs);
    if (bgfx::isValid(fs)) bgfx::destroy(fs);
    return ok;
}

// leaf cards discard below ref 8, a ref of 0 must not discard
bool alpha_discard_on() {
    return g_state_atest && g_alpha_ref > 0 && g_alpha_ref <= 32
        && (g_alpha_func == D3DCMP_GREATER || g_alpha_func == D3DCMP_GREATEREQUAL);
}

void ensure_shaders() {
    if (g_ready) return;
    auto type = bgfx::getRendererType();
    auto vs = bgfx::createEmbeddedShader(kTexVs, type, "vs_tex");
    auto fs = bgfx::createEmbeddedShader(kTexFs, type, "fs_tex");
    g_prog_tex = bgfx::createProgram(vs, fs, true);
    auto cvs = bgfx::createEmbeddedShader(kColVs, type, "vs_col");
    auto cfs = bgfx::createEmbeddedShader(kColFs, type, "fs_col");
    g_prog_col = bgfx::createProgram(cvs, cfs, true);
    auto mvs = bgfx::createEmbeddedShader(kModVs, type, "vs_mod");
    auto mfs = bgfx::createEmbeddedShader(kModFs, type, "fs_mod");
    g_prog_mod = bgfx::createProgram(mvs, mfs, true);
    auto take_at = [&](const bgfx::EmbeddedShader* vs_tab, const char* vs_name,
                       const bgfx::EmbeddedShader* fs_tab, const char* fs_name) {
        auto vsh = bgfx::createEmbeddedShader(vs_tab, type, vs_name);
        auto fsh = bgfx::createEmbeddedShader(fs_tab, type, fs_name);
        bgfx::ProgramHandle prog = BGFX_INVALID_HANDLE;
        if (bgfx::isValid(vsh) && bgfx::isValid(fsh))
            prog = bgfx::createProgram(vsh, fsh, true);
        else {
            if (bgfx::isValid(vsh)) bgfx::destroy(vsh);
            if (bgfx::isValid(fsh)) bgfx::destroy(fsh);
        }
        return prog;
    };
    // D3D11 has no DXBC blob, so this stays invalid and the draw keeps the plain program
    g_prog_tex_at = take_at(kTexVs, "vs_tex", kTexAtFs, "fs_tex_at");
    g_prog_mod_at = take_at(kModVs, "vs_mod", kModAtFs, "fs_mod_at");
    g_sampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
    g_sampler_mask = bgfx::createUniform("s_texMask", bgfx::UniformType::Sampler);
    g_mvp = bgfx::createUniform("u_mgModelViewPr", bgfx::UniformType::Mat4);
    g_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    g_layout_col.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    g_layout_mod.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    uint8_t white[4] = {255, 255, 255, 255};
    g_white = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP, bgfx::copy(white, 4));
    identity(g_world);
    identity(g_view);
    identity(g_proj);
    g_ready = bgfx::isValid(g_prog_tex) && bgfx::isValid(g_prog_col);
    static int logged_invalid = 0;
    if (g_ready || !logged_invalid) {
        std::cout << "bgfx programs tex=" << g_prog_tex.idx << " col=" << g_prog_col.idx
                  << " mod=" << g_prog_mod.idx
                  << " tex_at=" << g_prog_tex_at.idx << " mod_at=" << g_prog_mod_at.idx
                  << " renderer=" << bgfx::getRendererName(type) << "\n";
        if (!g_ready) logged_invalid = 1;
    }
}

uint32_t fvf_stride(uint32_t fvf) {
    uint32_t stride = 0;
    if (fvf & D3DFVF_XYZ) stride += 12;
    else if (fvf & 0x004) stride += 16; // XYZRHW
    if (fvf & D3DFVF_NORMAL) stride += 12;
    if (fvf & D3DFVF_DIFFUSE) stride += 4;
    if (fvf & 0x080) stride += 4; // specular
    uint32_t tex = (fvf & 0xf00) >> 8;
    uint32_t fmt = fvf >> 16;
    for (uint32_t i = 0; i < tex; ++i) {
        uint32_t size = (fmt >> (i * 2)) & 3;
        stride += (size == 0 ? 2 : size == 1 ? 3 : size == 2 ? 4 : 1) * 4;
    }
    return stride ? stride : 12;
}

struct Converted {
    std::vector<uint8_t> bytes;
    bool textured = false;
};

uint32_t scale_channel(uint32_t v, int shift) {
    if (shift > 0) v <<= shift;
    return v > 255 ? 255u : v;
}

uint32_t arg_color(uint32_t arg, uint32_t diffuse) {
    uint32_t sel = arg & 0xFu;
    uint32_t c = 0xFFFFFFFFu;
    if (sel == D3DTA_DIFFUSE || sel == D3DTA_CURRENT) c = diffuse;
    else if (sel == D3DTA_TFACTOR) c = g_tfactor;
    if (arg & D3DTA_COMPLEMENT) {
        c = ((255u - ((c >> 24) & 255u)) << 24) | ((255u - ((c >> 16) & 255u)) << 16)
          | ((255u - ((c >> 8) & 255u)) << 8) | (255u - (c & 255u));
    }
    return c;
}

uint32_t mul_color(uint32_t a, uint32_t b, int shift) {
    auto ch = [&](int sft) {
        uint32_t v = ((a >> sft) & 255u) * ((b >> sft) & 255u) >> 8;
        return scale_channel(v, sft == 24 ? 0 : shift);
    };
    return (ch(24) << 24) | (ch(16) << 16) | (ch(8) << 8) | ch(0);
}

// bake stage 0 into the vertex color, the shader only sees one combiner
uint32_t channel_arg(uint32_t arg, uint32_t diffuse, bool alpha_only) {
    uint32_t sel = arg & 0xFu;
    uint32_t c = 0xFFFFFFFFu;
    if (sel == D3DTA_DIFFUSE || sel == D3DTA_CURRENT) c = diffuse;
    else if (sel == D3DTA_TFACTOR) c = g_tfactor;
    else if (alpha_only && sel == D3DTA_TEXTURE) c = 0xFFFFFFFFu;
    if (arg & D3DTA_COMPLEMENT) {
        c = ((255u - ((c >> 24) & 255u)) << 24) | ((255u - ((c >> 16) & 255u)) << 16)
          | ((255u - ((c >> 8) & 255u)) << 8) | (255u - (c & 255u));
    }
    return c;
}

uint32_t bake_alpha(uint32_t diffuse) {
    uint32_t a = channel_arg(g_alpha_arg1, diffuse, true);
    uint32_t b = channel_arg(g_alpha_arg2, diffuse, true);
    uint32_t out = diffuse >> 24;
    if (g_alpha_op == D3DTOP_DISABLE) out = 255;
    else if (g_alpha_op == D3DTOP_SELECTARG1) out = a >> 24;
    else if (g_alpha_op == D3DTOP_SELECTARG2) out = b >> 24;
    else if (g_alpha_op == D3DTOP_MODULATE) out = ((a >> 24) * (b >> 24)) >> 8;
    else if (g_alpha_op == D3DTOP_MODULATE2X) out = std::min(255u, (((a >> 24) * (b >> 24)) >> 7));
    else if (g_alpha_op == D3DTOP_MODULATE4X) out = std::min(255u, (((a >> 24) * (b >> 24)) >> 6));
    return out > 255u ? 255u : out;
}

uint32_t bake_color(uint32_t diffuse) {
    uint32_t a = arg_color(g_color_arg1, diffuse);
    uint32_t b = arg_color(g_color_arg2, diffuse);
    uint32_t c = diffuse;
    if (g_color_op == D3DTOP_SELECTARG1) c = a;
    else if (g_color_op == D3DTOP_SELECTARG2) c = b;
    else if (g_color_op == D3DTOP_MODULATE) c = mul_color(a, b, 0);
    else if (g_color_op == D3DTOP_MODULATE2X) c = mul_color(a, b, 1);
    else if (g_color_op == D3DTOP_MODULATE4X) c = mul_color(a, b, 2);
    return (c & 0x00FFFFFFu) | (bake_alpha(diffuse) << 24);
}

Converted& convert_vertices(const uint8_t* src, uint32_t count, uint32_t fvf, uint32_t stride) {
    static Converted out;
    out.bytes.clear();
    out.textured = false;
    if (!src || count == 0) return out;
    if (stride == 0) stride = fvf_stride(fvf);
    const bool rhw = (fvf & 0x004) != 0;
    const bool xyz = (fvf & D3DFVF_XYZ) != 0 || rhw;
    const bool diffuse = (fvf & D3DFVF_DIFFUSE) != 0;
    uint32_t tex_count = (fvf & 0xf00) >> 8;
    if (tex_count > 0 && g_texture && g_texture->bgfx_tex == 0xFFFF && !g_texture->rgba.empty()) mgfx_texture_upload(g_texture);
    out.textured = tex_count > 0 && g_texture && g_texture->bgfx_tex != 0xFFFF;
    uint32_t out_stride = out.textured ? 24 : 16;
    out.bytes.resize(count * out_stride);
    int tw = g_target_w > 0 ? g_target_w : g_ScreenX;
    int th = g_target_h > 0 ? g_target_h : g_ScreenY;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* v = src + i * stride;
        uint32_t off = 0;
        float x = 0, y = 0, z = 0;
        if (xyz) {
            std::memcpy(&x, v + off, 4);
            std::memcpy(&y, v + off + 4, 4);
            std::memcpy(&z, v + off + 8, 4);
            off += rhw ? 16 : 12;
            if (rhw && tw > 0 && th > 0) {
                // drop the D3D9 -0.5 RHW bias, it samples the next texel here
                x = (x + 0.5f) / float(tw) * 2.f - 1.f;
                y = 1.f - (y + 0.5f) / float(th) * 2.f;
                z = 0.f;
            }
        }
        if (fvf & D3DFVF_NORMAL) off += 12;
        uint32_t color = 0xffffffff;
        if (diffuse) {
            std::memcpy(&color, v + off, 4);
            off += 4;
        }
        color = bake_color(color);
        if (g_tint_tfactor) {
            // fold the terrain factor into the vertex color
            uint32_t tf = g_tfactor;
            auto ch = [&](uint32_t shift) {
                return uint8_t((((color >> shift) & 255u) * ((tf >> shift) & 255u)) >> 8);
            };
            color = 0xFF000000u | (uint32_t(ch(16)) << 16) | (uint32_t(ch(8)) << 8) | uint32_t(ch(0));
        }
        if (fvf & 0x080) off += 4;
        float u = 0, tv = 0;
        if (tex_count) {
            // stage 0 may use UV set 1, the shoreline mask stays on set 0
            uint32_t set = g_water_draw ? 0u : (g_tex_index0 & 0xFFu);
            if (set >= tex_count) set = 0;
            uint32_t uvo = off + set * 8u;
            if (uvo + 8 > stride) uvo = off;
            std::memcpy(&u, v + uvo, 4);
            std::memcpy(&tv, v + uvo + 4, 4);
            // flip render targets only on OpenGL
            if (g_texture && g_texture->is_rt) {
                const bgfx::Caps* caps = bgfx::getCaps();
                if (caps && caps->originBottomLeft) tv = 1.f - tv;
            }
        }
        if (g_water_draw && tex_count >= 2 && g_mask_tex && g_mask_tex->width > 1
            && g_mask_tex->rgba.size() >= size_t(g_mask_tex->width) * g_mask_tex->height * 4) {
            float u1 = 0, v1 = 0;
            std::memcpy(&u1, v + off + 8, 4);
            std::memcpy(&v1, v + off + 12, 4);
            auto clamp01 = [](float s) { return s < 0.f ? 0.f : (s > 1.f ? 1.f : s); };
            uint32_t mx = uint32_t(clamp01(u) * float(g_mask_tex->width - 1));
            uint32_t my = uint32_t(clamp01(tv) * float(g_mask_tex->height - 1));
            uint8_t ma = g_mask_tex->rgba[(size_t(my) * g_mask_tex->width + mx) * 4 + 3];
            color = (color & 0x00FFFFFFu) | (uint32_t(ma) << 24);
            u = u1;
            tv = v1;
        }
        uint8_t* d = out.bytes.data() + i * out_stride;
        std::memcpy(d, &x, 4);
        std::memcpy(d + 4, &y, 4);
        std::memcpy(d + 8, &z, 4);
        d[12] = uint8_t(color >> 16);
        d[13] = uint8_t(color >> 8);
        d[14] = uint8_t(color);
        d[15] = uint8_t(color >> 24);
        if (out.textured) {
            std::memcpy(d + 16, &u, 4);
            std::memcpy(d + 20, &tv, 4);
        }
    }
    return out;
}

uint32_t prim_vertex_count(uint32_t prim, uint32_t prim_count) {
    if (prim == 4) return prim_count * 3;
    if (prim == 5) return prim_count + 2;
    if (prim == 6) return prim_count + 2;
    if (prim == 2) return prim_count * 2;
    if (prim == 3) return prim_count + 1;
    return prim_count;
}

void mul_row(const float* a, const float* b, float* o) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            o[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] + a[i * 4 + 1] * b[1 * 4 + j]
                         + a[i * 4 + 2] * b[2 * 4 + j] + a[i * 4 + 3] * b[3 * 4 + j];
}

// constructor preview is a panel rect, not the whole window
bool backbuffer_subview() {
    if (g_rt) return false;
    const int sw = g_ScreenX > 0 ? g_ScreenX : 1024;
    const int sh = g_ScreenY > 0 ? g_ScreenY : 768;
    if (g_vp.w == 0 || g_vp.h == 0) return false;
    if (g_vp.x == 0 && g_vp.y == 0) return false;
    if (int(g_vp.w) >= sw - 1 && int(g_vp.h) >= sh - 1) return false;
    return true;
}

// Map a full-window NDC box into the D3D viewport. mvp is row-major, v * M
void fit_backbuffer_viewport(float* mvp_row) {
    const float sw = float(g_ScreenX > 0 ? g_ScreenX : 1024);
    const float sh = float(g_ScreenY > 0 ? g_ScreenY : 768);
    const float sx = float(g_vp.w) / sw;
    const float sy = float(g_vp.h) / sh;
    const float ox = (2.f * float(g_vp.x) + float(g_vp.w)) / sw - 1.f;
    const float oy = 1.f - sy - (2.f * float(g_vp.y)) / sh;
    const float fit[16] = {
        sx, 0, 0, 0,
        0, sy, 0, 0,
        0, 0, 1, 0,
        ox, oy, 0, 1
    };
    float tmp[16];
    mul_row(mvp_row, fit, tmp);
    std::memcpy(mvp_row, tmp, sizeof(tmp));
}

void scissor_subview() {
    // HUD and minimap are already in pixels, do not scissor them away
    if ((g_fvf & 0x004) != 0) {
        bgfx::setScissor(UINT16_MAX);
        return;
    }
    if (!backbuffer_subview()) {
        bgfx::setScissor(UINT16_MAX);
        return;
    }
    bgfx::setScissor(uint16_t(g_vp.x), uint16_t(g_vp.y), uint16_t(g_vp.w), uint16_t(g_vp.h));
}

void adjust_clip(float* mvp_row) {
    const bgfx::Caps* caps = bgfx::getCaps();
    if (!caps) return;
    if (caps->homogeneousDepth) {
        float zfix[16] = {1,0,0,0, 0,1,0,0, 0,0,2,0, 0,0,-1,1};
        float tmp[16];
        mul_row(mvp_row, zfix, tmp);
        std::memcpy(mvp_row, tmp, sizeof(tmp));
    }
    // Vulkan's viewport is flipped, +Y is the top
}

bool read_index(const void* indices, bool index32, uint32_t ib_bytes, uint32_t n, uint32_t& out) {
    if (!indices) { out = n; return true; }
    const auto* bytes = static_cast<const uint8_t*>(indices);
    if (index32) {
        uint64_t off = uint64_t(n) * 4;
        if (ib_bytes && off + 4 > ib_bytes) return false;
        uint32_t s = 0;
        std::memcpy(&s, bytes + off, 4);
        out = s;
        return true;
    }
    uint64_t off = uint64_t(n) * 2;
    if (ib_bytes && off + 2 > ib_bytes) return false;
    uint16_t s = 0;
    std::memcpy(&s, bytes + off, 2);
    out = s;
    return true;
}

static bool texture_ready(IDirect3DTexture9* tex) {
    if (!tex || tex->width == 0 || tex->width > 8192 || tex->height == 0 || tex->height > 8192) return false;
    if (tex->is_rt && tex->bgfx_tex != 0xFFFF && bgfx::isValid(bgfx::TextureHandle{tex->bgfx_tex})) return true;
    if (tex->rgba.size() != size_t(tex->width) * tex->height * 4) return false;
    if (tex->bgfx_tex == 0xFFFF) mgfx_texture_upload(tex);
    return tex->bgfx_tex != 0xFFFF && bgfx::isValid(bgfx::TextureHandle{tex->bgfx_tex});
}

static int g_pick0 = 0, g_pick2 = 0, g_pick_other = 0, g_pick_none = 0, g_macro = 0;
static bool arg_is(uint32_t arg, uint32_t sel) { return (arg & 0xFu) == sel; }
static IDirect3DTexture9* pick_texture() {
    g_mask_tex = nullptr;
    g_tint_tfactor = 0;
    auto fresh = [](int s) {
        if (!g_stages[s] || g_stage_gen[s] == 0 || g_stage_gen[0] == 0) return false;
        uint32_t a = g_stage_gen[0], b = g_stage_gen[s];
        uint32_t d = a > b ? a - b : b - a;
        return d <= 8;
    };
    // shoreline: stage 0 is the mask, stage 1 is the water color
    if (g_water_draw && texture_ready(g_stages[1]) && g_stages[1]->width > 64) {
        if (texture_ready(g_stages[0])) g_mask_tex = g_stages[0];
        ++g_pick2;
        return g_stages[1];
    }
    // object gloss samples the albedo, not the dark specular map
    const bool additive = g_state_alpha && g_blend_src == D3DBLEND_ONE && g_blend_dst == D3DBLEND_ONE;
    auto op2_fresh = [&]() {
        if (g_op2_gen == 0 || g_stage_gen[0] == 0) return false;
        uint32_t a = g_stage_gen[0], b = g_op2_gen;
        uint32_t d = a > b ? a - b : b - a;
        return d <= 16;
    };
    const bool stage2_lit =
        (arg_is(g_color_arg1_2, D3DTA_TEXTURE) && (arg_is(g_color_arg2_2, D3DTA_DIFFUSE) || arg_is(g_color_arg2_2, D3DTA_TFACTOR)))
        || (arg_is(g_color_arg2_2, D3DTA_TEXTURE) && (arg_is(g_color_arg1_2, D3DTA_DIFFUSE) || arg_is(g_color_arg1_2, D3DTA_TFACTOR)));
    const bool gloss_albedo =
        !additive
        && g_color_op == D3DTOP_SELECTARG1 && arg_is(g_color_arg1, D3DTA_TEXTURE)
        && g_color_op2 == D3DTOP_MODULATE
        && stage2_lit
        && op2_fresh() && fresh(2) && texture_ready(g_stages[2]);
    const bool ui = (g_fvf & 0x004) != 0;
    if (gloss_albedo && !ui) {
        g_tint_tfactor = 1;
        ++g_pick_other;
        return g_stages[2];
    }
    if (texture_ready(g_stages[0]) && g_stages[0]->width < 8 && fresh(2) && texture_ready(g_stages[2]) && g_stages[1] && g_stages[1]->width >= 8 && fresh(1)) {
        ++g_pick2;
        return g_stages[2];
    }
    if (texture_ready(g_stages[0])) {
        ++g_pick0;
        return g_stages[0];
    }
    ++g_pick_none;
    return nullptr;
}

uint64_t blend_factor(uint32_t d3d) {
    switch (d3d) {
    case D3DBLEND_ZERO: return BGFX_STATE_BLEND_ZERO;
    case D3DBLEND_ONE: return BGFX_STATE_BLEND_ONE;
    case D3DBLEND_SRCCOLOR: return BGFX_STATE_BLEND_SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR: return BGFX_STATE_BLEND_INV_SRC_COLOR;
    case D3DBLEND_SRCALPHA: return BGFX_STATE_BLEND_SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA: return BGFX_STATE_BLEND_INV_SRC_ALPHA;
    case D3DBLEND_DESTALPHA: return BGFX_STATE_BLEND_DST_ALPHA;
    case D3DBLEND_INVDESTALPHA: return BGFX_STATE_BLEND_INV_DST_ALPHA;
    case D3DBLEND_DESTCOLOR: return BGFX_STATE_BLEND_DST_COLOR;
    case D3DBLEND_INVDESTCOLOR: return BGFX_STATE_BLEND_INV_DST_COLOR;
    default: return BGFX_STATE_BLEND_ONE;
    }
}

uint32_t stencil_op(uint32_t op, uint32_t zero, uint32_t keep, uint32_t replace, uint32_t incr, uint32_t incrsat, uint32_t decr, uint32_t decrsat, uint32_t invert) {
    switch (op) {
    case D3DSTENCILOP_ZERO: return zero;
    case D3DSTENCILOP_REPLACE: return replace;
    case D3DSTENCILOP_INCRSAT: return incrsat;
    case D3DSTENCILOP_DECRSAT: return decrsat;
    case D3DSTENCILOP_INVERT: return invert;
    case D3DSTENCILOP_INCR: return incr;
    case D3DSTENCILOP_DECR: return decr;
    default: return keep;
    }
}

uint32_t stencil_state() {
    uint32_t test = BGFX_STENCIL_TEST_ALWAYS;
    switch (g_stencil_func) {
    case D3DCMP_NEVER: test = BGFX_STENCIL_TEST_NEVER; break;
    case D3DCMP_LESS: test = BGFX_STENCIL_TEST_LESS; break;
    case D3DCMP_EQUAL: test = BGFX_STENCIL_TEST_EQUAL; break;
    case D3DCMP_LESSEQUAL: test = BGFX_STENCIL_TEST_LEQUAL; break;
    case D3DCMP_GREATER: test = BGFX_STENCIL_TEST_GREATER; break;
    case D3DCMP_NOTEQUAL: test = BGFX_STENCIL_TEST_NOTEQUAL; break;
    case D3DCMP_GREATEREQUAL: test = BGFX_STENCIL_TEST_GEQUAL; break;
    default: test = BGFX_STENCIL_TEST_ALWAYS; break;
    }
    uint32_t fail = stencil_op(g_stencil_fail,
        BGFX_STENCIL_OP_FAIL_S_ZERO, BGFX_STENCIL_OP_FAIL_S_KEEP, BGFX_STENCIL_OP_FAIL_S_REPLACE,
        BGFX_STENCIL_OP_FAIL_S_INCR, BGFX_STENCIL_OP_FAIL_S_INCRSAT, BGFX_STENCIL_OP_FAIL_S_DECR,
        BGFX_STENCIL_OP_FAIL_S_DECRSAT, BGFX_STENCIL_OP_FAIL_S_INVERT);
    uint32_t zfail = stencil_op(g_stencil_zfail,
        BGFX_STENCIL_OP_FAIL_Z_ZERO, BGFX_STENCIL_OP_FAIL_Z_KEEP, BGFX_STENCIL_OP_FAIL_Z_REPLACE,
        BGFX_STENCIL_OP_FAIL_Z_INCR, BGFX_STENCIL_OP_FAIL_Z_INCRSAT, BGFX_STENCIL_OP_FAIL_Z_DECR,
        BGFX_STENCIL_OP_FAIL_Z_DECRSAT, BGFX_STENCIL_OP_FAIL_Z_INVERT);
    uint32_t pass = stencil_op(g_stencil_pass,
        BGFX_STENCIL_OP_PASS_Z_ZERO, BGFX_STENCIL_OP_PASS_Z_KEEP, BGFX_STENCIL_OP_PASS_Z_REPLACE,
        BGFX_STENCIL_OP_PASS_Z_INCR, BGFX_STENCIL_OP_PASS_Z_INCRSAT, BGFX_STENCIL_OP_PASS_Z_DECR,
        BGFX_STENCIL_OP_PASS_Z_DECRSAT, BGFX_STENCIL_OP_PASS_Z_INVERT);
    uint32_t rmask = g_stencil_mask ? (g_stencil_mask & 0xffu) : 0xffu;
    return BGFX_STENCIL_FUNC_REF(g_stencil_ref & 0xffu) | BGFX_STENCIL_FUNC_RMASK(rmask) | test | fail | zfail | pass;
}

void submit(uint32_t prim, const uint8_t* verts, uint32_t stride, uint32_t vcount, const void* indices, uint32_t start, uint32_t prim_count, bool index32, int base_vertex, uint32_t ib_bytes) {
    const bool screen = (g_fvf & 0x004) != 0;
    const bool rgb_write = (g_color_write & 7) != 0;
    const bool alpha_write = (g_color_write & 8) != 0;
    // depth-only draws still have to run, water would otherwise fill the cell
    const bool depth_only = !rgb_write && !alpha_write && g_state_z && g_state_zwrite
        && !g_stencil && !g_state_alpha && !screen;
    // stencil volumes write no color and blend zero/one, they still update the mask
    if (!depth_only && !g_stencil && ((!rgb_write && !alpha_write) ||
        (g_state_alpha && g_blend_src == D3DBLEND_ZERO && g_blend_dst == D3DBLEND_ONE))) {
        ++g_skipped;
        return;
    }
    // do not draw the mask as a color, except the shoreline
    const bool select_current =
        (g_color_op == D3DTOP_SELECTARG1 && (g_color_arg1 & 0xF) == D3DTA_CURRENT) ||
        (g_color_op == D3DTOP_SELECTARG2 && (g_color_arg2 & 0xF) == D3DTA_CURRENT);
    g_water_draw = 0;
    const bool water = select_current && g_state_alpha && g_stages[0]
        && g_stages[0]->width == 64 && g_stages[0]->height == 64
        && g_stages[1] && g_stages[1]->width > 64;
    if (select_current && g_stages[0] && !water) {
        ++g_skipped;
        return;
    }
    if (water) g_water_draw = 1;
    ensure_shaders();
    if (!g_ready || !verts || vcount == 0 || stride < 12 || prim_count == 0) return;
    struct TexGuard { IDirect3DTexture9* saved; ~TexGuard() { g_texture = saved; } } guard{g_texture};
    g_texture = pick_texture();
    const bool lines = prim == 2 || prim == 3;
    const uint32_t step = lines ? 2u : 3u;

    static std::vector<uint32_t> idx;
    idx.clear();
    idx.reserve(std::min<uint32_t>(prim_count, 8192) * step);
    auto accept = [&](uint32_t slot, uint32_t& abs) -> bool {
        uint32_t raw = 0;
        if (!read_index(indices, index32, ib_bytes, slot, raw)) return false;
        int v = int(raw) + base_vertex;
        if (v < 0 || uint32_t(v) >= vcount) return false;
        float p[3];
        std::memcpy(p, verts + size_t(v) * stride, 12);
        if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])) return false;
        abs = uint32_t(v);
        return true;
    };
    if (!indices && (prim == 5 || prim == 6 || prim == 3)) {
        for (uint32_t p = 0; p < prim_count; ++p) {
            uint32_t a, b, c;
            if (prim == 3) {
                if (!accept(p, a) || !accept(p + 1, b)) continue;
                idx.push_back(a); idx.push_back(b);
            } else if (!accept(0, a) && prim == 6) {
                break;
            } else if (prim == 6) {
                if (!accept(0, a) || !accept(p + 1, b) || !accept(p + 2, c)) continue;
                idx.push_back(a); idx.push_back(b); idx.push_back(c);
            } else {
                if (!accept(p, a) || !accept(p + 1, b) || !accept(p + 2, c)) continue;
                if (p & 1) { idx.push_back(a); idx.push_back(c); idx.push_back(b); }
                else { idx.push_back(a); idx.push_back(b); idx.push_back(c); }
            }
        }
    } else if (indices && (prim == 5 || prim == 6 || prim == 3)) {
        for (uint32_t p = 0; p < prim_count; ++p) {
            uint32_t a, b, c;
            if (prim == 3) {
                if (!accept(start + p, a) || !accept(start + p + 1, b)) continue;
                idx.push_back(a); idx.push_back(b);
            } else if (prim == 6) {
                if (!accept(start, a) || !accept(start + p + 1, b) || !accept(start + p + 2, c)) continue;
                idx.push_back(a); idx.push_back(b); idx.push_back(c);
            } else {
                if (!accept(start + p, a) || !accept(start + p + 1, b) || !accept(start + p + 2, c)) continue;
                if (p & 1) { idx.push_back(a); idx.push_back(c); idx.push_back(b); }
                else { idx.push_back(a); idx.push_back(b); idx.push_back(c); }
            }
        }
    } else if (prim == 4 || prim == 2 || !indices) {
        uint32_t nprim = prim_count;
        if (!indices && prim != 4 && prim != 2 && prim != 1) nprim = prim_count;
        for (uint32_t p = 0; p < nprim; ++p) {
            if (lines || prim == 2) {
                uint32_t a, b;
                uint32_t base = indices ? start + p * 2 : p * 2;
                if (!accept(base, a) || !accept(base + 1, b)) continue;
                idx.push_back(a); idx.push_back(b);
            } else {
                uint32_t a, b, c;
                uint32_t base = indices ? start + p * 3 : p * 3;
                if (!accept(base, a) || !accept(base + 1, b) || !accept(base + 2, c)) continue;
                idx.push_back(a); idx.push_back(b); idx.push_back(c);
            }
        }
    }
    if (idx.empty()) return;

    float mvp_row[16], wv[16], idn[16];
    identity(idn);
    if (screen) identity(mvp_row);
    else {
        float view[16];
        std::memcpy(view, g_view, sizeof(view));
        mul_row(g_world, view, wv);
        mul_row(wv, g_proj, mvp_row);
        adjust_clip(mvp_row);
        if (backbuffer_subview()) fit_backbuffer_viewport(mvp_row);
    }

    uint32_t cursor = 0;
    static std::vector<uint32_t> used;
    static std::vector<uint16_t> ib;
    static std::vector<uint8_t> tight;
    static std::vector<uint8_t> fallback;
    while (cursor + step <= idx.size()) {
        used.clear();
        ib.clear();
        static std::vector<uint32_t> stamp;
        static std::vector<uint16_t> remap;
        static uint32_t gen = 1;
        if (stamp.size() < vcount) {
            stamp.assign(vcount, 0);
            remap.assign(vcount, 0);
            gen = 1;
        }
        if (++gen == 0) {
            std::fill(stamp.begin(), stamp.end(), 0);
            gen = 1;
        }
        uint32_t end = cursor;
        for (; end + step <= idx.size(); end += step) {
            uint32_t add = 0;
            for (uint32_t k = 0; k < step; ++k)
                if (stamp[idx[end + k]] != gen) ++add;
            if (used.size() + add > 60000 && !ib.empty()) break;
            for (uint32_t k = 0; k < step; ++k) {
                uint32_t v = idx[end + k];
                if (stamp[v] != gen) {
                    stamp[v] = gen;
                    remap[v] = uint16_t(used.size());
                    used.push_back(v);
                }
            }
            for (uint32_t k = 0; k < step; ++k) ib.push_back(remap[idx[end + k]]);
        }
        if (used.empty() || ib.empty()) break;
        tight.resize(used.size() * stride);
        for (size_t i = 0; i < used.size(); ++i)
            std::memcpy(tight.data() + i * stride, verts + size_t(used[i]) * stride, stride);
        auto& converted = convert_vertices(tight.data(), uint32_t(used.size()), g_fvf, stride);
        if (converted.bytes.empty()) { cursor = end; continue; }
        const bool mod_ok = converted.textured && bgfx::isValid(g_prog_mod);
        fallback.clear();
        const uint8_t* vbytes = converted.bytes.data();
        const bgfx::VertexLayout* layout = &g_layout_col;
        uint32_t out_stride = 16;
        bgfx::ProgramHandle prog = g_prog_col;
        if (mod_ok) {
            layout = &g_layout_mod;
            out_stride = 24;
            prog = (alpha_discard_on() && bgfx::isValid(g_prog_mod_at)) ? g_prog_mod_at : g_prog_mod;
        } else if (converted.textured && bgfx::isValid(g_prog_tex)) {
            fallback.resize(used.size() * 20);
            for (size_t i = 0; i < used.size(); ++i) {
                const uint8_t* s = converted.bytes.data() + i * 24;
                uint8_t* d = fallback.data() + i * 20;
                std::memcpy(d, s, 12);
                std::memcpy(d + 12, s + 16, 8);
            }
            vbytes = fallback.data();
            layout = &g_layout;
            out_stride = 20;
            prog = (alpha_discard_on() && bgfx::isValid(g_prog_tex_at)) ? g_prog_tex_at : g_prog_tex;
        }
        const bool textured = mod_ok || !fallback.empty();
        uint32_t out_count = uint32_t(used.size());
        // keep the sky gradient transparent or it hides the clouds
        // feather the lower edge, a flat alpha leaves a hard horizon bar
        if (screen && !textured && !g_state_z && g_state_alpha && g_ScreenX > 0 && !converted.bytes.empty()) {
            float xmin = 1e9f, xmax = -1e9f;
            float ymin = 1e9f, ymax = -1e9f;
            for (size_t i = 0; i < used.size(); ++i) {
                float x = 0;
                std::memcpy(&x, tight.data() + i * stride, 4);
                if (x < xmin) xmin = x;
                if (x > xmax) xmax = x;
                float y = 0;
                std::memcpy(&y, converted.bytes.data() + i * 16 + 4, 4);
                if (y < ymin) ymin = y;
                if (y > ymax) ymax = y;
            }
            if (xmax - xmin > float(g_ScreenX) * 0.75f) {
                const float span = ymax - ymin;
                const bool feather = span > 0.05f && span < 1.2f;
                const float band = span * 0.45f;
                for (size_t i = 0; i < used.size(); ++i) {
                    uint8_t* a = converted.bytes.data() + i * 16 + 15;
                    uint32_t av = (uint32_t(*a) * 110u) / 255u;
                    if (feather && band > 1e-4f) {
                        float y = 0;
                        std::memcpy(&y, converted.bytes.data() + i * 16 + 4, 4);
                        float t = (y - ymin) / band;
                        if (t < 0.f) t = 0.f;
                        if (t > 1.f) t = 1.f;
                        t = t * t * (3.f - 2.f * t);
                        av = uint32_t(float(av) * t + 0.5f);
                    }
                    *a = uint8_t(av > 255u ? 255u : av);
                }
            }
        }
        const bool transient = bgfx::getAvailTransientVertexBuffer(out_count, *layout) >= out_count
                            && bgfx::getAvailTransientIndexBuffer(uint32_t(ib.size())) >= ib.size();
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;
        bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle ibh = BGFX_INVALID_HANDLE;
        if (transient) {
            bgfx::allocTransientVertexBuffer(&tvb, out_count, *layout);
            std::memcpy(tvb.data, vbytes, size_t(out_count) * out_stride);
            bgfx::allocTransientIndexBuffer(&tib, uint32_t(ib.size()));
            std::memcpy(tib.data, ib.data(), ib.size() * sizeof(uint16_t));
        } else {
            ++g_dropped;
            vbh = bgfx::createVertexBuffer(bgfx::copy(vbytes, out_count * out_stride), *layout);
            ibh = bgfx::createIndexBuffer(bgfx::copy(ib.data(), uint32_t(ib.size() * sizeof(uint16_t))));
            if (!bgfx::isValid(vbh) || !bgfx::isValid(ibh)) {
                if (bgfx::isValid(vbh)) bgfx::destroy(vbh);
                if (bgfx::isValid(ibh)) bgfx::destroy(ibh);
                cursor = end;
                continue;
            }
        }
        bgfx::setViewTransform(g_draw_view, idn, idn);
        bgfx::setTransform(idn);
        if (bgfx::isValid(g_mvp)) bgfx::setUniform(g_mvp, screen ? idn : mvp_row);
        if (transient) {
            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setIndexBuffer(&tib);
        } else {
            bgfx::setVertexBuffer(0, vbh);
            bgfx::setIndexBuffer(ibh);
        }
        uint64_t state = 0;
        if (rgb_write) state |= BGFX_STATE_WRITE_RGB;
        if (alpha_write) state |= BGFX_STATE_WRITE_A;
        if (lines) state |= BGFX_STATE_PT_LINES;
        // honor Z disable, or the sky fills the depth buffer
        if (!screen && g_state_z) {
            state |= BGFX_STATE_DEPTH_TEST_LEQUAL;
            if (g_state_zwrite) state |= BGFX_STATE_WRITE_Z;
        }
        if (g_state_alpha) state |= BGFX_STATE_BLEND_FUNC(blend_factor(g_blend_src), blend_factor(g_blend_dst));
        // D3DCULL_CW drops clockwise, D3DCULL_CCW drops counter-clockwise
        if (!lines) {
            if (g_cull == 2) state |= BGFX_STATE_CULL_CW;
            else if (g_cull == 3) state |= BGFX_STATE_CULL_CCW;
        }
        bgfx::setState(state);
        if (g_stencil) bgfx::setStencil(stencil_state());
        else bgfx::setStencil(BGFX_STENCIL_NONE);
        scissor_subview();
        if (textured && g_texture && bgfx::isValid(bgfx::TextureHandle{g_texture->bgfx_tex})) {
            uint32_t samp = 0;
            if (g_mag_filter <= 1) samp |= BGFX_SAMPLER_POINT | BGFX_SAMPLER_MIP_POINT;
            if (!g_water_draw && (g_addr_u == 3 || g_addr_v == 3))
                samp |= BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
            bgfx::setTexture(0, g_sampler, bgfx::TextureHandle{g_texture->bgfx_tex}, samp);
            bgfx::TextureHandle mask = g_white;
            // water samples the color with the second UV, not the mask
            if (!g_water_draw && g_mask_tex && g_mask_tex->bgfx_tex != 0xFFFF && bgfx::isValid(bgfx::TextureHandle{g_mask_tex->bgfx_tex}))
                mask = bgfx::TextureHandle{g_mask_tex->bgfx_tex};
            if (mod_ok && bgfx::isValid(g_sampler_mask) && bgfx::isValid(mask))
                bgfx::setTexture(1, g_sampler_mask, mask, g_water_draw ? 0u : samp);
        }
        bgfx::submit(g_draw_view, prog);
        // blend the macro texture or water tiles show as blue squares
        const bool macro = mod_ok && !screen && !g_water_draw && g_state_z && g_state_zwrite
            && !g_state_alpha && g_color_op == D3DTOP_SELECTARG1 && (g_color_arg1 & 0xF) == D3DTA_TEXTURE
            && g_color_op1 == D3DTOP_BLENDTEXTUREALPHA && ((g_fvf >> 8) & 0xF) >= 2
            && texture_ready(g_stages[1]);
        if (macro && bgfx::getAvailTransientVertexBuffer(out_count, g_layout_mod) >= out_count
            && bgfx::getAvailTransientIndexBuffer(uint32_t(ib.size())) >= ib.size()) {
            uint32_t uvo = 0;
            if (g_fvf & D3DFVF_XYZ) uvo += 12;
            else if (g_fvf & 0x004) uvo += 16;
            if (g_fvf & D3DFVF_NORMAL) uvo += 12;
            if (g_fvf & D3DFVF_DIFFUSE) uvo += 4;
            if (g_fvf & 0x080) uvo += 4;
            uvo += 8;
            if (uvo + 8 <= stride) {
                static std::vector<uint8_t> macv;
                macv.resize(size_t(out_count) * 24);
                for (uint32_t i = 0; i < out_count; ++i) {
                    std::memcpy(macv.data() + size_t(i) * 24, converted.bytes.data() + size_t(i) * 24, 16);
                    std::memcpy(macv.data() + size_t(i) * 24 + 16, tight.data() + size_t(i) * stride + uvo, 8);
                    macv[size_t(i) * 24 + 15] = 255;
                }
                bgfx::TransientVertexBuffer mtb;
                bgfx::TransientIndexBuffer mib;
                bgfx::allocTransientVertexBuffer(&mtb, out_count, g_layout_mod);
                bgfx::allocTransientIndexBuffer(&mib, uint32_t(ib.size()));
                if (mtb.data && mib.data) {
                    std::memcpy(mtb.data, macv.data(), macv.size());
                    std::memcpy(mib.data, ib.data(), ib.size() * sizeof(uint16_t));
                    bgfx::setViewTransform(g_draw_view, idn, idn);
                    bgfx::setTransform(idn);
                    if (bgfx::isValid(g_mvp)) bgfx::setUniform(g_mvp, mvp_row);
                    bgfx::setVertexBuffer(0, &mtb);
                    bgfx::setIndexBuffer(&mib);
                    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LEQUAL
                        | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA));
                    bgfx::setTexture(0, g_sampler, bgfx::TextureHandle{g_stages[1]->bgfx_tex}, 0);
                    if (bgfx::isValid(g_sampler_mask) && bgfx::isValid(g_white))
                        bgfx::setTexture(1, g_sampler_mask, g_white, 0);
                    scissor_subview();
                    bgfx::submit(g_draw_view, g_prog_mod);
                    ++g_macro;
                }
            }
        }
        // fog start/end are eye-space distances
        if (!screen && g_fog && g_state_z && g_fog_end > g_fog_start + 1.f && bgfx::isValid(g_prog_mod) && bgfx::isValid(g_white)) {
            static std::vector<uint8_t> fogv;
            fogv.resize(size_t(out_count) * 24);
            float max_a = 0.f;
            float fog_start = g_fog_start;
            float fog_end = g_fog_end;
            // pull fog start in, the visible ground is closer than 2000
            if (fog_end > 400.f && fog_start > fog_end * 0.35f) fog_start = fog_end * 0.28f;
            const float span = fog_end - fog_start;
            const uint8_t fr = uint8_t(g_fog_color >> 16);
            const uint8_t fg = uint8_t(g_fog_color >> 8);
            const uint8_t fb = uint8_t(g_fog_color);
            for (uint32_t i = 0; i < out_count; ++i) {
                float x, y, z;
                std::memcpy(&x, vbytes + size_t(i) * out_stride, 4);
                std::memcpy(&y, vbytes + size_t(i) * out_stride + 4, 4);
                std::memcpy(&z, vbytes + size_t(i) * out_stride + 8, 4);
                // eye-space distance comes from gl_Position.w
                float ez = x * mvp_row[3] + y * mvp_row[7] + z * mvp_row[11] + mvp_row[15];
                if (ez < 0.f) ez = -ez;
                float f = (fog_end - ez) / span;
                if (f < 0.f) f = 0.f;
                if (f > 1.f) f = 1.f;
                float a = 1.f - f;
                if (a > max_a) max_a = a;
                uint8_t* d = fogv.data() + size_t(i) * 24;
                std::memcpy(d, &x, 4);
                std::memcpy(d + 4, &y, 4);
                std::memcpy(d + 8, &z, 4);
                d[12] = fr; d[13] = fg; d[14] = fb; d[15] = uint8_t(a * 255.f);
                float u = 0.f;
                std::memcpy(d + 16, &u, 4);
                std::memcpy(d + 20, &u, 4);
            }
            if (max_a > 0.03f && bgfx::getAvailTransientVertexBuffer(out_count, g_layout_mod) >= out_count
                && bgfx::getAvailTransientIndexBuffer(uint32_t(ib.size())) >= ib.size()) {
                bgfx::TransientVertexBuffer fvb;
                bgfx::TransientIndexBuffer fib;
                bgfx::allocTransientVertexBuffer(&fvb, out_count, g_layout_mod);
                bgfx::allocTransientIndexBuffer(&fib, uint32_t(ib.size()));
                if (fvb.data && fib.data) {
                    std::memcpy(fvb.data, fogv.data(), fogv.size());
                    std::memcpy(fib.data, ib.data(), ib.size() * sizeof(uint16_t));
                    bgfx::setVertexBuffer(0, &fvb);
                    bgfx::setIndexBuffer(&fib);
                    bgfx::setTransform(idn);
                    if (bgfx::isValid(g_mvp)) bgfx::setUniform(g_mvp, mvp_row);
                    uint64_t fst = BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_LEQUAL
                        | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
                    if (lines) fst |= BGFX_STATE_PT_LINES;
                    bgfx::setState(fst);
                    bgfx::setTexture(0, g_sampler, g_white);
                    if (bgfx::isValid(g_sampler_mask)) bgfx::setTexture(1, g_sampler_mask, g_white);
                    scissor_subview();
                    bgfx::submit(g_draw_view, g_prog_mod);
                }
            }
        }
        if (!transient) {
            bgfx::destroy(vbh);
            bgfx::destroy(ibh);
        }
        ++g_draws;
        if (textured) ++g_tex_draws;
        cursor = end;
    }
}

void reset_renderer_state() {
    // clear g_ready, the next battle must not reuse dead handles
    g_ready = false;
    g_prog_tex = BGFX_INVALID_HANDLE;
    g_prog_col = BGFX_INVALID_HANDLE;
    g_prog_mod = BGFX_INVALID_HANDLE;
    g_prog_tex_at = BGFX_INVALID_HANDLE;
    g_prog_mod_at = BGFX_INVALID_HANDLE;
    g_sampler = BGFX_INVALID_HANDLE;
    g_sampler_mask = BGFX_INVALID_HANDLE;
    g_mvp = BGFX_INVALID_HANDLE;
    g_white = BGFX_INVALID_HANDLE;
    g_texture = nullptr;
    g_mask_tex = nullptr;
    for (int i = 0; i < 8; ++i) g_stages[i] = nullptr;
    g_frames = 0;
    g_draws = 0;
    g_tex_draws = 0;
    g_skipped = 0;
    g_dropped = 0;
    g_pick0 = g_pick2 = g_pick_other = g_pick_none = g_macro = 0;
    g_vb = nullptr;
    g_ib = nullptr;
    g_rt = nullptr;
    g_cpu_target = nullptr;
    g_draw_view = VIEW_WORLD;
    invalidate_live_textures();
    std::cout << "bgfx device reset\n";
}
}  // namespace

void mgfx_reset_device() { reset_renderer_state(); }

// toast texture, destroyed with the device
static bgfx::TextureHandle g_screen_image = BGFX_INVALID_HANDLE;
static int g_screen_w = 0;
static int g_screen_h = 0;

// bgfx::shutdown joins the render thread and frees the device
void mgfx_shutdown_device() {
    if (bgfx::getCaps()) {
        if (bgfx::isValid(g_screen_image))
            bgfx::destroy(g_screen_image);
        g_screen_image = BGFX_INVALID_HANDLE;
        g_screen_w = 0;
        g_screen_h = 0;
        bgfx::shutdown();
    } else {
        g_screen_image = BGFX_INVALID_HANDLE;
        g_screen_w = 0;
        g_screen_h = 0;
    }
    reset_renderer_state();
    std::cout << "battle device stopped\n";
}

// host RGBA toast, ID3DXFont cannot draw it
void mgfx_draw_screen_rgba(int x, int y, int w, int h, const void* rgba, int pitch) {
    if (!rgba || w < 1 || h < 1 || w > 4096 || h > 2048) return;
    if (!bgfx::getCaps()) return;
    ensure_shaders();
    ensure_views();
    if (!g_ready) return;
    const int src_pitch = pitch > 0 ? pitch : w * 4;
    std::vector<uint8_t> tight(size_t(w) * h * 4);
    const auto* src = static_cast<const uint8_t*>(rgba);
    for (int row = 0; row < h; ++row)
        std::memcpy(tight.data() + size_t(row) * w * 4, src + size_t(row) * src_pitch, size_t(w) * 4);
    const uint64_t flags = BGFX_SAMPLER_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    if (!bgfx::isValid(g_screen_image) || g_screen_w != w || g_screen_h != h) {
        if (bgfx::isValid(g_screen_image)) bgfx::destroy(g_screen_image);
        g_screen_image = bgfx::createTexture2D(uint16_t(w), uint16_t(h), false, 1, bgfx::TextureFormat::RGBA8,
            flags, bgfx::copy(tight.data(), uint32_t(tight.size())));
        g_screen_w = w;
        g_screen_h = h;
    } else {
        bgfx::updateTexture2D(g_screen_image, 0, 0, 0, 0, uint16_t(w), uint16_t(h),
            bgfx::copy(tight.data(), uint32_t(tight.size())));
    }
    if (!bgfx::isValid(g_screen_image)) return;
    const bool mod = bgfx::isValid(g_prog_mod);
    const bgfx::VertexLayout& layout = mod ? g_layout_mod : g_layout;
    if (bgfx::getAvailTransientVertexBuffer(4, layout) < 4 || bgfx::getAvailTransientIndexBuffer(6) < 6) return;
    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
    bgfx::allocTransientIndexBuffer(&tib, 6);
    if (!tvb.data || !tib.data) return;
    const float sw = g_ScreenX > 0 ? float(g_ScreenX) : 1024.f;
    const float sh = g_ScreenY > 0 ? float(g_ScreenY) : 768.f;
    const float x0 = float(x) / sw * 2.f - 1.f;
    const float x1 = float(x + w) / sw * 2.f - 1.f;
    const float y0 = 1.f - float(y) / sh * 2.f;
    const float y1 = 1.f - float(y + h) / sh * 2.f;
    const float pos[4][2] = {{x0, y0}, {x1, y0}, {x0, y1}, {x1, y1}};
    const float uv[4][2] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};
    auto* bytes = static_cast<uint8_t*>(tvb.data);
    const uint32_t stride = mod ? 24u : 20u;
    for (int i = 0; i < 4; ++i) {
        uint8_t* d = bytes + i * stride;
        std::memcpy(d, &pos[i][0], 4);
        std::memcpy(d + 4, &pos[i][1], 4);
        float z = 0.f;
        std::memcpy(d + 8, &z, 4);
        if (mod) {
            d[12] = d[13] = d[14] = d[15] = 255;
            std::memcpy(d + 16, uv[i], 8);
        } else {
            std::memcpy(d + 12, uv[i], 8);
        }
    }
    auto* ib = reinterpret_cast<uint16_t*>(tib.data);
    ib[0] = 0; ib[1] = 1; ib[2] = 2; ib[3] = 2; ib[4] = 1; ib[5] = 3;
    float idn[16] = {};
    idn[0] = idn[5] = idn[10] = idn[15] = 1.f;
    bgfx::setViewTransform(VIEW_UI, idn, idn);
    bgfx::setTransform(idn);
    if (bgfx::isValid(g_mvp)) bgfx::setUniform(g_mvp, idn);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setTexture(0, g_sampler, g_screen_image);
    if (mod && bgfx::isValid(g_sampler_mask) && bgfx::isValid(g_white))
        bgfx::setTexture(1, g_sampler_mask, g_white);
    bgfx::setScissor(UINT16_MAX);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA);
    bgfx::submit(VIEW_UI, mod ? g_prog_mod : g_prog_tex);
}

extern "C" void mg_on_key(int vk, int down);
long mgfx_present() {
    ensure_shaders();
    ensure_views();
    bgfx::touch(VIEW_WORLD);
    bgfx::touch(VIEW_UI);
    // clear depth so LEQUAL terrain is not rejected, and stencil so volumes start from 0
    uint16_t bits = BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL;
    if (g_clear_color) bits |= BGFX_CLEAR_COLOR;
    bgfx::setViewClear(VIEW_WORLD, bits, g_clear_rgba, 1.0f, 0);
    g_clear_color = false;
    bgfx::frame();
    ++g_frames;
    if (g_frames <= 3 || (g_frames % 120) == 0) {
        std::cout << "frame " << g_frames << " draws " << g_draws << " textured " << g_tex_draws
                  << " pick0 " << g_pick0 << " pick2 " << g_pick2 << " other " << g_pick_other << " none " << g_pick_none
                  << " skip " << g_skipped << " drop " << g_dropped << " macro " << g_macro
                  << std::endl;
    }
    return S_OK;
}
static int g_rt_cursor = 0;

static void ensure_rt_fb(IDirect3DTexture9* tex) {
    if (!tex || tex->fb != 0xFFFF) return;
    ensure_shaders();
    if (!bgfx::getCaps()) return;
    uint16_t w = uint16_t(tex->width ? tex->width : 1);
    uint16_t h = uint16_t(tex->height ? tex->height : 1);
    const uint64_t flags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    auto color = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA8, flags);
    if (!bgfx::isValid(color)) {
        static int fails = 0;
        if (fails++ < 4) std::cout << "rt color fail " << w << "x" << h << "\n";
        return;
    }
    bgfx::TextureFormat::Enum depths[] = {
        bgfx::TextureFormat::D24S8, bgfx::TextureFormat::D32F, bgfx::TextureFormat::D16
    };
    bgfx::TextureHandle depth = BGFX_INVALID_HANDLE;
    for (auto fmt : depths) {
        if (!bgfx::isTextureValid(0, false, 1, fmt, BGFX_TEXTURE_RT | BGFX_TEXTURE_RT_WRITE_ONLY)) continue;
        depth = bgfx::createTexture2D(w, h, false, 1, fmt, BGFX_TEXTURE_RT | BGFX_TEXTURE_RT_WRITE_ONLY);
        if (bgfx::isValid(depth)) break;
    }
    bgfx::FrameBufferHandle fb = BGFX_INVALID_HANDLE;
    if (bgfx::isValid(depth)) {
        bgfx::TextureHandle att[2] = {color, depth};
        fb = bgfx::createFrameBuffer(2, att, false);
    }
    if (!bgfx::isValid(fb)) {
        if (bgfx::isValid(depth)) bgfx::destroy(depth);
        fb = bgfx::createFrameBuffer(1, &color, false);
    }
    if (!bgfx::isValid(fb)) {
        bgfx::destroy(color);
        static int fails = 0;
        if (fails++ < 4) std::cout << "rt fb fail " << w << "x" << h << "\n";
        return;
    }
    tex->bgfx_tex = color.idx;
    tex->fb = fb.idx;
    tex->is_rt = true;
    static int made = 0;
    if (made++ < 6) std::cout << "rt fb " << w << "x" << h << " tex " << color.idx << "\n";
}

static uint16_t ensure_rt_view(IDirect3DTexture9* tex) {
    ensure_rt_fb(tex);
    if (!tex || tex->fb == 0xFFFF) return VIEW_WORLD;
    if (tex->rt_frame == int(g_frames) && tex->rt_view >= 0) return uint16_t(tex->rt_view);
    if (g_rt_cursor >= 15) g_rt_cursor = 0;
    uint16_t view = uint16_t(g_rt_cursor++);
    tex->rt_view = int(view);
    tex->rt_frame = int(g_frames);
    bgfx::setViewFrameBuffer(view, bgfx::FrameBufferHandle{tex->fb});
    bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
    bgfx::setViewClear(view, BGFX_CLEAR_NONE, 0, 1.f, 0);
    uint16_t w = uint16_t(tex->width), h = uint16_t(tex->height);
    bgfx::setViewRect(view, 0, 0, w, h);
    return view;
}

long mgfx_clear(uint32_t flags, uint32_t color, float z) {
    if (g_cpu_target && g_cpu_target->owner && (flags & 1)) {
        auto* tex = g_cpu_target->owner;
        size_t n = size_t(tex->width) * tex->height * 4;
        if (tex->rgba.size() < n) tex->rgba.assign(n, 0);
        uint8_t rr = uint8_t(color >> 16), gg = uint8_t(color >> 8), bb = uint8_t(color), aa = uint8_t(color >> 24);
        for (size_t i = 0; i + 3 < n; i += 4) {
            tex->rgba[i] = rr; tex->rgba[i + 1] = gg; tex->rgba[i + 2] = bb; tex->rgba[i + 3] = aa;
        }
        return S_OK;
    }
    uint32_t r = (color >> 16) & 255, g = (color >> 8) & 255, b = color & 255, a = (color >> 24) & 255;
    // a zero-alpha clear is opaque on the screen, transparent on a shadow map
    if (!a && !(g_rt && g_rt->shadow_target)) a = 255;
    uint32_t rgba = (r << 24) | (g << 16) | (b << 8) | a;
    if (g_rt && g_rt->owner && g_rt->owner->is_rt) {
        uint16_t view = ensure_rt_view(g_rt->owner);
        uint16_t bits = 0;
        if (flags & 1) bits |= BGFX_CLEAR_COLOR;
        if (flags & 2) bits |= BGFX_CLEAR_DEPTH;
        if (flags & 4) bits |= BGFX_CLEAR_STENCIL;
        float dz = z <= 0.f ? 1.f : z;
        bgfx::setViewClear(view, bits ? bits : uint16_t(BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH), rgba, dz, 0);
        bgfx::touch(view);
        return S_OK;
    }
    if (g_rt) {
        if (g_rt->pixels.size() < size_t(g_rt->width) * g_rt->height * 4)
            g_rt->pixels.assign(size_t(g_rt->width) * g_rt->height * 4, 0);
        if (g_rt->depth.size() < size_t(g_rt->width) * g_rt->height)
            g_rt->depth.assign(size_t(g_rt->width) * g_rt->height, 1.f);
        if (flags & 1) {
            uint8_t rb = uint8_t(color), gg = uint8_t(color >> 8), rr = uint8_t(color >> 16), aa = uint8_t(color >> 24);
            if (!aa && !g_rt->shadow_target) aa = 255;
            for (size_t i = 0; i + 3 < g_rt->pixels.size(); i += 4) {
                g_rt->pixels[i] = rr; g_rt->pixels[i + 1] = gg; g_rt->pixels[i + 2] = rb; g_rt->pixels[i + 3] = aa;
            }
        }
        if (flags & 2) {
            float dz = z <= 0.f ? 1.f : z;
            std::fill(g_rt->depth.begin(), g_rt->depth.end(), dz);
            if (!g_rt->depth_keep.empty()) std::fill(g_rt->depth_keep.begin(), g_rt->depth_keep.end(), dz);
        }
        return S_OK;
    }
    if (!(flags & 1) && z <= 0.f) return S_OK;
    uint16_t bits = 0;
    if (flags & 1) { bits |= BGFX_CLEAR_COLOR; g_clear_color = true; g_clear_rgba = rgba; }
    if (flags & 2) bits |= BGFX_CLEAR_DEPTH;
    if (flags & 4) bits |= BGFX_CLEAR_STENCIL;
    float dz = (!(flags & 1) || z <= 0.f) ? 1.f : z;
    bgfx::setViewClear(VIEW_WORLD, bits ? bits : uint16_t(BGFX_CLEAR_DEPTH), rgba, dz, 0);
    bgfx::touch(VIEW_WORLD);
    return S_OK;
}

void mgfx_set_render_state(uint32_t state, uint32_t value) {
    if (state == D3DRS_ALPHABLENDENABLE) g_state_alpha = value;
    else if (state == D3DRS_ALPHATESTENABLE) g_state_atest = value;
    else if (state == D3DRS_ALPHAREF) g_alpha_ref = value;
    else if (state == D3DRS_ALPHAFUNC) g_alpha_func = value;
    else if (state == 7) g_state_z = value;
    else if (state == D3DRS_ZWRITEENABLE) g_state_zwrite = value;
    else if (state == 22) g_cull = value;
    else if (state == D3DRS_SRCBLEND) g_blend_src = value;
    else if (state == D3DRS_DESTBLEND) g_blend_dst = value;
    else if (state == D3DRS_COLORWRITEENABLE) g_color_write = value;
    else if (state == D3DRS_STENCILENABLE) g_stencil = value;
    else if (state == D3DRS_STENCILFUNC) g_stencil_func = value;
    else if (state == D3DRS_STENCILFAIL) g_stencil_fail = value;
    else if (state == D3DRS_STENCILZFAIL) g_stencil_zfail = value;
    else if (state == D3DRS_STENCILPASS) g_stencil_pass = value;
    else if (state == D3DRS_STENCILREF) g_stencil_ref = value;
    else if (state == D3DRS_STENCILMASK) g_stencil_mask = value;
    else if (state == D3DRS_STENCILWRITEMASK) g_stencil_wmask = value;
    else if (state == D3DRS_TEXTUREFACTOR) g_tfactor = value;
    else if (state == D3DRS_FOGENABLE) g_fog = value;
    else if (state == D3DRS_FOGCOLOR) g_fog_color = value;
    else if (state == D3DRS_FOGSTART) std::memcpy(&g_fog_start, &value, 4);
    else if (state == D3DRS_FOGEND) std::memcpy(&g_fog_end, &value, 4);
}

void mgfx_set_sampler(uint32_t stage, uint32_t type, uint32_t value) {
    if (stage != 0) return;
    if (type == D3DSAMP_MAGFILTER || type == D3DSAMP_MINFILTER) g_mag_filter = value;
    else if (type == D3DSAMP_ADDRESSU) g_addr_u = value;
    else if (type == D3DSAMP_ADDRESSV) g_addr_v = value;
}

void mgfx_set_tss(uint32_t stage, uint32_t type, uint32_t value) {
    if (stage == 0 && type == D3DTSS_TEXCOORDINDEX) g_tex_index0 = value & 0xFFFFu;
    if (stage == 1 && type == D3DTSS_COLOROP) g_color_op1 = value;
    if (stage == 2 && type == D3DTSS_COLOROP) {
        g_color_op2 = value;
        g_op2_gen = g_tex_serial;
    }
    else if (stage == 2 && type == D3DTSS_COLORARG1) g_color_arg1_2 = value;
    else if (stage == 2 && type == D3DTSS_COLORARG2) g_color_arg2_2 = value;
    if (stage != 0) return;
    if (type == D3DTSS_COLOROP) g_color_op = value;
    else if (type == D3DTSS_COLORARG1) g_color_arg1 = value;
    else if (type == D3DTSS_COLORARG2) g_color_arg2 = value;
    else if (type == D3DTSS_ALPHAOP) g_alpha_op = value;
    else if (type == D3DTSS_ALPHAARG1) g_alpha_arg1 = value;
    else if (type == D3DTSS_ALPHAARG2) g_alpha_arg2 = value;
}

void mgfx_set_transform(uint32_t which, const void* matrix) {
    if (!matrix) return;
    // texture matrices are not the world matrix
    float* dst = nullptr;
    if (which == D3DTS_VIEW) dst = g_view;
    else if (which == D3DTS_PROJECTION) dst = g_proj;
    else if (which == D3DTS_WORLD) dst = g_world;
    else return;
    std::memcpy(dst, matrix, sizeof(float) * 16);
}

void mgfx_set_texture(uint32_t stage, IDirect3DTexture9* tex) {
    if (stage < 8) {
        g_stages[stage] = tex;
        g_stage_gen[stage] = ++g_tex_serial;
    }
    if (stage == 0) g_texture = tex;
    if (tex && tex->is_rt) return;
    if (stage == 0 && tex) {
        static int bad = 0;
        if (!texture_ready(tex) && bad < 6) {
            ++bad;
            std::cout << "badtex " << tex->width << "x" << tex->height
                      << " rgba " << tex->rgba.size() << " handle " << tex->bgfx_tex << "\n";
        }
    }
    if (tex && !tex->is_rt && tex->width > 0 && tex->width <= 8192 && tex->height > 0 && tex->height <= 8192
        && tex->rgba.size() == size_t(tex->width) * tex->height * 4
        && tex->bgfx_tex == 0xFFFF) mgfx_texture_upload(tex);
}

void mgfx_set_fvf(uint32_t fvf) {
    g_fvf = fvf;
    g_stride = fvf_stride(fvf);
}

void mgfx_set_stream(IDirect3DVertexBuffer9* vb, uint32_t offset, uint32_t stride) {
    g_vb = vb;
    g_vb_offset = offset;
    if (stride) g_stride = stride;
}

void mgfx_set_render_target(IDirect3DSurface9* surface) {
    g_rt = (surface && (surface->shadow_target || surface->owner)) ? surface : nullptr;
}

void mgfx_set_viewport(const D3DVIEWPORT9* vp) {
    if (!vp) return;
    g_vp.x = vp->X; g_vp.y = vp->Y; g_vp.w = vp->Width; g_vp.h = vp->Height;
    g_vp.minz = vp->MinZ; g_vp.maxz = vp->MaxZ;
    if (g_rt && g_rt->owner && g_rt->owner->is_rt) {
        uint16_t view = ensure_rt_view(g_rt->owner);
        uint16_t w = g_vp.w ? uint16_t(g_vp.w) : uint16_t(g_rt->owner->width);
        uint16_t h = g_vp.h ? uint16_t(g_vp.h) : uint16_t(g_rt->owner->height);
        bgfx::setViewRect(view, uint16_t(g_vp.x), uint16_t(g_vp.y), w ? w : 1, h ? h : 1);
    }
}

void mgfx_get_viewport(D3DVIEWPORT9* vp) {
    if (!vp) return;
    vp->X = g_vp.w ? g_vp.x : 0;
    vp->Y = g_vp.w ? g_vp.y : 0;
    vp->Width = g_vp.w ? g_vp.w : uint32_t(g_ScreenX > 0 ? g_ScreenX : 1024);
    vp->Height = g_vp.h ? g_vp.h : uint32_t(g_ScreenY > 0 ? g_ScreenY : 768);
    vp->MinZ = g_vp.minz;
    vp->MaxZ = g_vp.maxz;
}

static void raster_cpu(uint32_t prim, const uint8_t* src, uint32_t stride, uint32_t vcount, const void* indices, uint32_t start, uint32_t prim_count, bool index32, int base_vertex, uint32_t ib_bytes) {
    if (!g_rt || !src || stride < 12 || !g_rt->width || !g_rt->height || prim_count == 0) return;
    if (g_rt->width > 512 || g_rt->height > 512) return;
    const bool rgb_write = (g_color_write & 7) != 0;
    const bool alpha_write = (g_color_write & 8) != 0;
    // clipper writes only depth into the shadow target
    if (!rgb_write && !alpha_write && !(g_state_z && g_state_zwrite)) {
        ++g_skipped;
        return;
    }
    if (!g_stencil && g_state_alpha && g_blend_src == D3DBLEND_ZERO && g_blend_dst == D3DBLEND_ONE) {
        ++g_skipped;
        return;
    }
    const size_t pixels_n = size_t(g_rt->width) * g_rt->height;
    if (g_rt->pixels.size() < pixels_n * 4) g_rt->pixels.assign(pixels_n * 4, 0);
    if (g_rt->depth.size() < pixels_n) g_rt->depth.assign(pixels_n, 1.f);
    // clipper writes depth with color off, the silhouette must test that buffer
    const bool clip_depth = !rgb_write && !alpha_write && g_state_z && g_state_zwrite;
    if (clip_depth) {
        if (g_rt->depth_keep.size() < pixels_n) g_rt->depth_keep.assign(pixels_n, 1.f);
    }
    IDirect3DTexture9* saved = g_texture;
    IDirect3DTexture9* tex = pick_texture();
    g_texture = saved;
    float wv[16], mvp[16];
    mul_row(g_world, g_view, wv);
    mul_row(wv, g_proj, mvp);
    const bool rhw = (g_fvf & 0x004) != 0;
    const int sw = int(g_rt->width), sh = int(g_rt->height);
    auto load_index = [&](uint32_t slot, uint32_t& abs) -> bool {
        uint32_t raw = 0;
        if (!read_index(indices, index32, ib_bytes, slot, raw)) return false;
        int v = int(raw) + base_vertex;
        if (v < 0 || uint32_t(v) >= vcount) return false;
        abs = uint32_t(v);
        return true;
    };
    struct Vtx { float sx, sy, sz, d, u, tv; uint8_t r, g, b, a; bool ok; };
    auto fetch2 = [&](uint32_t vi) -> Vtx {
        Vtx o{};
        const uint8_t* v = src + size_t(vi) * stride;
        uint32_t off = rhw ? 16u : 12u;
        float x = 0, y = 0, z = 0;
        std::memcpy(&x, v, 4);
        std::memcpy(&y, v + 4, 4);
        std::memcpy(&z, v + 8, 4);
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return o;
        if (g_fvf & D3DFVF_NORMAL) off += 12;
        uint32_t color = 0xffffffff;
        if (g_fvf & D3DFVF_DIFFUSE) { std::memcpy(&color, v + off, 4); off += 4; }
        if (g_fvf & 0x080) off += 4;
        float u = 0, tv = 0;
        if ((g_fvf & 0xf00) && off + 8 <= stride) {
            std::memcpy(&u, v + off, 4);
            std::memcpy(&tv, v + off + 4, 4);
        }
        o.r = uint8_t(color >> 16); o.g = uint8_t(color >> 8); o.b = uint8_t(color); o.a = uint8_t(color >> 24);
        if (!o.a) o.a = 255;
        if (rhw) { o.sx = x; o.sy = y; o.sz = 1; o.d = z; o.u = u; o.tv = tv; o.ok = true; return o; }
        float X = x*mvp[0] + y*mvp[4] + z*mvp[8] + mvp[12];
        float Y = x*mvp[1] + y*mvp[5] + z*mvp[9] + mvp[13];
        float Z = x*mvp[2] + y*mvp[6] + z*mvp[10] + mvp[14];
        float W = x*mvp[3] + y*mvp[7] + z*mvp[11] + mvp[15];
        if (!(W > 0.05f) || !std::isfinite(W)) return o;
        float inv = 1.f / W;
        o.sx = (X * inv * 0.5f + 0.5f) * float(sw);
        o.sy = (0.5f - Y * inv * 0.5f) * float(sh);
        o.sz = inv;
        o.d = Z * inv;
        o.u = u * inv;
        o.tv = tv * inv;
        o.ok = std::isfinite(o.sx) && std::isfinite(o.sy);
        return o;
    };
    auto sample = [&](float u, float tv, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        if (!tex || tex->width == 0 || tex->rgba.size() < size_t(tex->width) * tex->height * 4) return;
        float uu = u - std::floor(u);
        float vv = tv - std::floor(tv);
        uint32_t x = uint32_t(uu * float(tex->width)) % tex->width;
        uint32_t y = uint32_t(vv * float(tex->height)) % tex->height;
        const uint8_t* p = tex->rgba.data() + (size_t(y) * tex->width + x) * 4;
        r = uint8_t((uint32_t(r) * p[0]) >> 8);
        g = uint8_t((uint32_t(g) * p[1]) >> 8);
        b = uint8_t((uint32_t(b) * p[2]) >> 8);
        a = uint8_t((uint32_t(a) * p[3]) >> 8);
    };
    const bool lines = prim == 2 || prim == 3;
    // shadow pass is TFACTOR color. Opaque skins take TFACTOR alpha, cards take texture alpha
    const bool shadow_pass = g_rt->shadow_target && !clip_depth
        && g_color_op == D3DTOP_SELECTARG1 && (g_color_arg1 & 0xF) == D3DTA_TFACTOR
        && g_alpha_op == D3DTOP_SELECTARG1;
    const bool shadow_cut = shadow_pass && g_state_atest && (g_alpha_arg1 & 0xF) == D3DTA_TEXTURE;
    const bool silhouette = shadow_pass && (shadow_cut || (g_alpha_arg1 & 0xF) == D3DTA_TFACTOR);
    auto paint = [&](const Vtx& a, const Vtx& b, const Vtx& c) {
        if (!a.ok || !b.ok || !c.ok) return;
        int minx = std::max(0, int(std::floor(std::min(a.sx, std::min(b.sx, c.sx)))));
        int maxx = std::min(sw - 1, int(std::ceil(std::max(a.sx, std::max(b.sx, c.sx)))));
        int miny = std::max(0, int(std::floor(std::min(a.sy, std::min(b.sy, c.sy)))));
        int maxy = std::min(sh - 1, int(std::ceil(std::max(a.sy, std::max(b.sy, c.sy)))));
        if (maxx < minx || maxy < miny) return;
        auto edge = [](float ax, float ay, float bx, float by, float cx, float cy) {
            return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
        };
        float area = edge(a.sx, a.sy, b.sx, b.sy, c.sx, c.sy);
        if (std::fabs(area) < 1e-4f) return;
        // screen y is flipped, so CW/CCW swap against the D3D winding
        if (!lines && g_cull == 2 && area > 0) return;
        if (!lines && g_cull == 3 && area < 0) return;
        for (int py = miny; py <= maxy; ++py) {
            for (int px = minx; px <= maxx; ++px) {
                float fx = float(px) + 0.5f, fy = float(py) + 0.5f;
                float w0 = edge(b.sx, b.sy, c.sx, c.sy, fx, fy);
                float w1 = edge(c.sx, c.sy, a.sx, a.sy, fx, fy);
                float w2 = edge(a.sx, a.sy, b.sx, b.sy, fx, fy);
                if (area > 0) { if (w0 < 0 || w1 < 0 || w2 < 0) continue; }
                else { if (w0 > 0 || w1 > 0 || w2 > 0) continue; }
                float b0 = w0 / area, b1 = w1 / area, b2 = w2 / area;
                float inv = b0 * a.sz + b1 * b.sz + b2 * c.sz;
                if (!(inv > 1e-8f)) continue;
                float u = (b0 * a.u + b1 * b.u + b2 * c.u) / inv;
                float tv = (b0 * a.tv + b1 * b.tv + b2 * c.tv) / inv;
                float depth = (b0 * a.d * a.sz + b1 * b.d * b.sz + b2 * c.d * c.sz) / inv;
                size_t pi = size_t(py) * sw + px;
                if (g_state_z) {
                    if (depth > g_rt->depth[pi]) continue;
                    if (g_state_zwrite) g_rt->depth[pi] = depth;
                    if (clip_depth) g_rt->depth_keep[pi] = depth;
                } else if (silhouette && g_rt->depth_keep.size() == pixels_n && depth > g_rt->depth_keep[pi]) {
                    continue;
                }
                uint8_t r = uint8_t((b0 * a.r + b1 * b.r + b2 * c.r));
                uint8_t g = uint8_t((b0 * a.g + b1 * b.g + b2 * c.g));
                uint8_t bb = uint8_t((b0 * a.b + b1 * b.b + b2 * c.b));
                uint8_t aa = uint8_t((b0 * a.a + b1 * b.a + b2 * c.a));
                if (shadow_cut || !silhouette) sample(u, tv, r, g, bb, aa);
                // card shadows test the caller's ref, leaf discard on the main view stays at ref 32
                if (shadow_cut) { if (aa < g_alpha_ref) continue; }
                else if (alpha_discard_on() && aa < g_alpha_ref) continue;
                uint8_t* dst = g_rt->pixels.data() + pi * 4;
                if (silhouette && rgb_write && alpha_write) {
                    dst[0] = dst[1] = dst[2] = 255;
                    dst[3] = 255;
                    continue;
                }
                if (g_state_alpha && aa < 255) {
                    uint32_t ia = aa;
                    if (rgb_write) {
                        dst[0] = uint8_t((r * ia + dst[0] * (255 - ia)) >> 8);
                        dst[1] = uint8_t((g * ia + dst[1] * (255 - ia)) >> 8);
                        dst[2] = uint8_t((bb * ia + dst[2] * (255 - ia)) >> 8);
                    }
                    if (alpha_write) dst[3] = uint8_t((aa * ia + dst[3] * (255 - ia)) >> 8);
                } else {
                    if (rgb_write) { dst[0] = r; dst[1] = g; dst[2] = bb; }
                    if (alpha_write) dst[3] = aa;
                }
            }
        }
    };
    if (lines && !clip_depth) return;
    auto stamp_line = [&](const Vtx& a, const Vtx& b) {
        if (!a.ok || !b.ok) return;
        int x0 = int(std::lround(a.sx)), y0 = int(std::lround(a.sy));
        int x1 = int(std::lround(b.sx)), y1 = int(std::lround(b.sy));
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, steps = std::max(dx, -dy);
        if (steps < 1) steps = 1;
        for (int i = 0; i <= steps; ++i) {
            if (x0 >= 0 && y0 >= 0 && x0 < sw && y0 < sh && g_rt->depth_keep.size() == pixels_n) {
                float t = float(i) / float(steps);
                float depth = a.d + (b.d - a.d) * t;
                size_t pi = size_t(y0) * sw + x0;
                if (depth <= g_rt->depth_keep[pi]) g_rt->depth_keep[pi] = depth;
            }
            if (x0 == x1 && y0 == y1) break;
            int e2 = err * 2;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    };
    auto tri = [&](uint32_t i0, uint32_t i1, uint32_t i2) {
        uint32_t a, b, c;
        if (!load_index(i0, a) || !load_index(i1, b) || !load_index(i2, c)) return;
        if (lines) { stamp_line(fetch2(a), fetch2(b)); return; }
        paint(fetch2(a), fetch2(b), fetch2(c));
    };
    if (!indices && (prim == 5 || prim == 6)) {
        for (uint32_t p = 0; p < prim_count; ++p) {
            if (prim == 6) tri(0, p + 1, p + 2);
            else if (p & 1) tri(p, p + 2, p + 1);
            else tri(p, p + 1, p + 2);
        }
    } else if (indices && (prim == 5 || prim == 6)) {
        for (uint32_t p = 0; p < prim_count; ++p) {
            if (prim == 6) tri(start, start + p + 1, start + p + 2);
            else if (p & 1) tri(start + p, start + p + 2, start + p + 1);
            else tri(start + p, start + p + 1, start + p + 2);
        }
    } else {
        for (uint32_t p = 0; p < prim_count; ++p) {
            uint32_t base = indices ? start + p * 3 : p * 3;
            tri(base, base + 1, base + 2);
        }
    }
    ++g_draws;
}

void mgfx_set_indices(IDirect3DIndexBuffer9* ib) { g_ib = ib; }

long mgfx_draw(uint32_t prim, int base_vertex, uint32_t start_index, uint32_t prim_count, const void* verts, uint32_t stride, uint32_t vcount, const void* indices, uint32_t icount, bool index32) {
    const bool screen = (g_fvf & 0x004) != 0;
    g_draw_view = screen ? VIEW_UI : VIEW_WORLD;
    g_target_w = 0;
    g_target_h = 0;
    if (g_rt && g_rt->owner && g_rt->owner->is_rt) {
        g_draw_view = ensure_rt_view(g_rt->owner);
        g_target_w = g_vp.w ? int(g_vp.w) : int(g_rt->owner->width);
        g_target_h = g_vp.h ? int(g_vp.h) : int(g_rt->owner->height);
    } else if (g_rt) {
        // CPU readback for the robot portrait
    } else {
        g_rt = nullptr;
        // constructor preview draws into the panel, not the battlefield
        if (!screen && backbuffer_subview()) g_draw_view = VIEW_UI;
    }
    if (prim_count == 0) return S_OK;
    if (prim_count > 300000) prim_count = 300000;
    const uint8_t* src = static_cast<const uint8_t*>(verts);
    uint32_t stride_use = stride ? stride : g_stride;
    if (!stride_use) stride_use = fvf_stride(g_fvf);
    uint32_t ib_bytes = 0;
    if (icount == 0xFFFFFFFFu) {
        if (!g_ib || g_ib->bytes.empty()) return S_OK;
        indices = g_ib->bytes.data();
        index32 = g_ib->index32;
        ib_bytes = uint32_t(g_ib->bytes.size());
    }
    const bool indexed = indices != nullptr;
    if (!src && g_vb) {
        if (g_vb->bytes.empty() || g_vb_offset >= g_vb->bytes.size() || stride_use == 0) return S_OK;
        src = g_vb->bytes.data() + g_vb_offset;
        uint32_t vb_verts = uint32_t((g_vb->bytes.size() - g_vb_offset) / stride_use);
        if (!indexed) {
            uint32_t start = base_vertex > 0 ? uint32_t(base_vertex) : 0;
            uint32_t need = prim_vertex_count(prim, prim_count);
            if (start >= vb_verts || need == 0) return S_OK;
            if (start + need > vb_verts) need = vb_verts - start;
            src += size_t(start) * stride_use;
            vcount = need;
            base_vertex = 0;
            start_index = 0;
        } else {
            vcount = vb_verts;
        }
    } else if (src && !indexed && vcount == 0) {
        vcount = prim_vertex_count(prim, prim_count);
    }
    if (!src || vcount == 0 || stride_use < 12) return S_OK;
    if (g_rt && !(g_rt->owner && g_rt->owner->is_rt)) {
        raster_cpu(prim, src, stride_use, vcount, indexed ? indices : nullptr, start_index, prim_count, index32, base_vertex, ib_bytes);
        return S_OK;
    }
    submit(prim, src, stride_use, vcount, indexed ? indices : nullptr, start_index, prim_count, index32, base_vertex, ib_bytes);
    return S_OK;
}

void mgfx_texture_upload(IDirect3DTexture9* tex) {
    if (!tex || tex->is_rt || tex->rgba.empty() || tex->width == 0 || tex->height == 0) return;
    if (tex->width > 8192 || tex->height > 8192) return;
    if (tex->rgba.size() < size_t(tex->width) * tex->height * 4) return;
    if (!bgfx::getCaps()) return;
    static int dumped = 0;
    if (dumped < 4) {
        uint32_t nz = 0;
        const uint32_t n = tex->width * tex->height;
        for (uint32_t i = 0; i < n; ++i) {
            const uint8_t* p = tex->rgba.data() + i * 4;
            if (p[0] | p[1] | p[2]) ++nz;
        }
        const uint32_t mid = ((tex->height / 2) * tex->width + tex->width / 2) * 4;
        const uint8_t* c = tex->rgba.data() + std::min<uint32_t>(mid, uint32_t(tex->rgba.size() - 4));
        std::cout << "upload " << tex->width << "x" << tex->height << " nonzero " << nz << "/" << n
                  << " mid " << int(c[0]) << "," << int(c[1]) << "," << int(c[2]) << "," << int(c[3]) << "\n";
        if (dumped == 0) {
            FILE* f = std::fopen("/tmp/tex0.ppm", "wb");
            if (f) {
                std::fprintf(f, "P6\n%u %u\n255\n", tex->width, tex->height);
                for (uint32_t i = 0; i < n; ++i) {
                    const uint8_t* p = tex->rgba.data() + i * 4;
                    std::fputc(p[0], f); std::fputc(p[1], f); std::fputc(p[2], f);
                }
                std::fclose(f);
            }
        }
        ++dumped;
    }
    bool cpu_text = g_cpu_target && g_cpu_target->owner == tex;
    if (!cpu_text) {
        bool any_alpha = false;
        for (size_t i = 3; i < tex->rgba.size(); i += 4) {
            if (tex->rgba[i]) { any_alpha = true; break; }
        }
        if (!any_alpha) {
            // opaque images stored with alpha 0 need alpha, a blank canvas does not
            bool varied = false;
            const uint8_t r0 = tex->rgba[0], g0 = tex->rgba[1], b0 = tex->rgba[2];
            for (size_t i = 0; i + 2 < tex->rgba.size(); i += 4) {
                if (tex->rgba[i] != r0 || tex->rgba[i + 1] != g0 || tex->rgba[i + 2] != b0) {
                    varied = true;
                    break;
                }
            }
            if (varied) {
                for (size_t i = 3; i < tex->rgba.size(); i += 4) tex->rgba[i] = 255;
            }
        }
    }
    // lift near-black icon cutouts, vegetation cards are larger and their dark edge must stay dark
    if (!cpu_text && tex->width <= 64 && tex->height <= 64) {
        const uint32_t n = tex->width * tex->height;
        uint32_t mask_px = 0, opaque = 0, soft = 0;
        for (uint32_t i = 0; i < n; ++i) {
            const uint8_t* p = tex->rgba.data() + i * 4;
            if (p[3] > 240) ++opaque;
            const bool dark = p[0] < 8 && p[1] < 8 && p[2] < 8;
            if (p[3] > 8 && dark) ++mask_px;
            if (p[3] > 8 && p[3] < 220 && dark) ++soft;
        }
        // lift binary cutouts only, a soft alpha is a scorch
        if (n && mask_px * 5 > n && opaque * 3 < n * 2 && soft * 2 < mask_px) {
            for (uint32_t i = 0; i < n; ++i) {
                uint8_t* p = tex->rgba.data() + i * 4;
                if (p[3] > 220 && p[0] < 8 && p[1] < 8 && p[2] < 8) p[0] = p[1] = p[2] = 255;
            }
            static int lifted = 0;
            if (lifted < 12) {
                ++lifted;
                std::cout << "alpha-lift " << tex->width << "x" << tex->height << "\n";
            }
        }
    }
    const uint32_t bytes = tex->width * tex->height * 4;
    auto mem = bgfx::copy(tex->rgba.data(), bytes);
    // recreate the texture, updateTexture2D is ignored
    auto existing = bgfx::TextureHandle{tex->bgfx_tex};
    if (bgfx::isValid(existing)) bgfx::destroy(existing);
    auto handle = bgfx::createTexture2D(uint16_t(tex->width), uint16_t(tex->height), false, 1, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_NONE | BGFX_TEXTURE_NONE, mem);
    tex->bgfx_tex = bgfx::isValid(handle) ? handle.idx : uint16_t(0xFFFF);
}

static bool decode_png(const void* data, uint32_t size, IDirect3DTexture9* tex) {
    if (!data || size < 8) return false;
    const auto* bytes = static_cast<const uint8_t*>(data);
    if (bytes[0] != 0x89 || bytes[1] != 'P' || bytes[2] != 'N' || bytes[3] != 'G') return false;
    png_image image;
    std::memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_memory(&image, data, size)) return false;
    image.format = PNG_FORMAT_RGBA;
    tex->width = image.width ? image.width : 1;
    tex->height = image.height ? image.height : 1;
    tex->rgba.resize(tex->width * tex->height * 4);
    bool ok = png_image_finish_read(&image, nullptr, tex->rgba.data(), 0, nullptr) != 0;
    png_image_free(&image);
    if (!ok) return false;
    mgfx_texture_upload(tex);
    return true;
}

long mgfx_texture_from_memory(const void* data, uint32_t size, IDirect3DTexture9** out) {
    if (!out) return E_FAIL;
    auto* tex = new IDirect3DTexture9();
    *out = tex;
    auto note = [&](const char* how) {
        static int n = 0;
        if (n >= 24) return;
        ++n;
        const auto* b = data && size >= 4 ? static_cast<const uint8_t*>(data) : nullptr;
        std::printf("tex#%d %s size %u magic %02x%02x%02x%02x -> %ux%u\n", n, how, size,
                    b ? b[0] : 0, b ? b[1] : 0, b ? b[2] : 0, b ? b[3] : 0, tex->width, tex->height);
        std::fflush(stdout);
    };
    if (!data || size < 8) { note("empty"); return S_OK; }
    if (decode_png(data, size, tex)) { note("png"); return S_OK; }
    // decode 16-bit lum-alpha DDS here, bimg does not
    if (size >= 128) {
        const auto* b = static_cast<const uint8_t*>(data);
        if (b[0] == 'D' && b[1] == 'D' && b[2] == 'S' && b[3] == ' ') {
            uint32_t h = 0, w = 0, flags = 0, bitcount = 0;
            std::memcpy(&h, b + 12, 4);
            std::memcpy(&w, b + 16, 4);
            std::memcpy(&flags, b + 80, 4);
            std::memcpy(&bitcount, b + 88, 4);
            const uint8_t* src = b + 128;
            const uint32_t n = w * h;
            const bool lum = (flags & 0x20000) != 0;
            if (lum && w > 0 && h > 0 && w <= 4096 && h <= 4096 && bitcount == 16 && (flags & 1)
                && size >= 128u + n * 2u) {
                tex->width = w;
                tex->height = h;
                tex->rgba.resize(size_t(n) * 4);
                for (uint32_t i = 0; i < n; ++i) {
                    uint8_t L = src[i * 2], A = src[i * 2 + 1];
                    uint8_t* p = tex->rgba.data() + size_t(i) * 4;
                    p[0] = p[1] = p[2] = L;
                    p[3] = A;
                }
                mgfx_texture_upload(tex);
                note("l8a8");
                return S_OK;
            }
            if (lum && w > 0 && h > 0 && w <= 4096 && h <= 4096 && bitcount == 8 && !(flags & 1)
                && size >= 128u + n) {
                tex->width = w;
                tex->height = h;
                tex->rgba.resize(size_t(n) * 4);
                for (uint32_t i = 0; i < n; ++i) {
                    uint8_t L = src[i];
                    uint8_t* p = tex->rgba.data() + size_t(i) * 4;
                    p[0] = p[1] = p[2] = L;
                    p[3] = 255;
                }
                mgfx_texture_upload(tex);
                note("l8");
                return S_OK;
            }
        }
    }
    bx::Error err;
    bimg::ImageContainer* image = bimg::imageParseDds(&g_alloc, data, size, &err);
    if (!image) {
        note("fail");
        return S_OK;
    }
    tex->width = image->m_width;
    tex->height = image->m_height;
    bimg::ImageContainer* rgba = image;
    if (image->m_format != bimg::TextureFormat::RGBA8) {
        rgba = bimg::imageConvert(&g_alloc, bimg::TextureFormat::RGBA8, *image, false);
    }
    if (rgba && rgba->m_data) {
        uint32_t bytes = rgba->m_width * rgba->m_height * 4;
        tex->rgba.assign(static_cast<uint8_t*>(rgba->m_data), static_cast<uint8_t*>(rgba->m_data) + bytes);
        mgfx_texture_upload(tex);
        note("dds");
    } else {
        note("convert-fail");
    }
    if (rgba && rgba != image) bimg::imageFree(rgba);
    bimg::imageFree(image);
    return S_OK;
}

namespace {
IDirect3DDevice9* g_device = nullptr;
}

long IDirect3D9::CreateDevice(uint32_t, uint32_t, HWND, uint32_t, D3DPRESENT_PARAMETERS* params, IDirect3DDevice9** device) {
    if (!g_device) g_device = new IDirect3DDevice9();
    if (device) *device = g_device;
    uint32_t width = params && params->BackBufferWidth ? params->BackBufferWidth : 1280;
    uint32_t height = params && params->BackBufferHeight ? params->BackBufferHeight : 720;
    bgfx::setViewClear(VIEW_WORLD, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x1a2430ff, 1.0f, 0);
    bgfx::setViewRect(VIEW_WORLD, 0, 0, uint16_t(width), uint16_t(height));
    ensure_shaders();
    std::cout << "device " << width << "x" << height << "\n";
    return D3D_OK;
}

IDirect3D9* Direct3DCreate9(uint32_t) { return new IDirect3D9(); }

long D3DXCreateFontW(IDirect3DDevice9*, int height, uint32_t, uint32_t, uint32_t, int, uint32_t, uint32_t, uint32_t, uint32_t, const wchar_t*, ID3DXFont** font) {
    if (font) {
        *font = new ID3DXFont();
        (*font)->height = height > 0 ? height : 13;
    }
    return S_OK;
}

int mgfx_draw_text(const wchar_t*, int, void*, uint32_t, uint32_t, int) {
    return 0;
}

