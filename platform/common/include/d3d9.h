// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once
// D3D9 surface, implemented on bgfx

#include "windows.h"
#include <cstdint>
#include <cstring>
#include <vector>

using D3DCOLOR = uint32_t;

struct IDirect3DTexture9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;
struct IDirect3DSurface9;
long mgfx_present();
long mgfx_begin_scene();
long mgfx_clear(uint32_t flags, uint32_t color, float z);
void mgfx_set_render_state(uint32_t state, uint32_t value);
void mgfx_set_tss(uint32_t stage, uint32_t type, uint32_t value);
void mgfx_begin_cpu_target(IDirect3DSurface9* surface);
void mgfx_end_cpu_target();
void mgfx_set_transform(uint32_t which, const void* matrix);
void mgfx_set_texture(uint32_t stage, IDirect3DTexture9* tex);
void mgfx_set_sampler(uint32_t stage, uint32_t type, uint32_t value);
uint32_t mgfx_get_sampler(uint32_t stage, uint32_t type);
void mgfx_set_anisotropy(int degree);
void mgfx_apply_present(uint32_t width, uint32_t height, uint32_t samples, uint32_t interval);
void mgfx_set_fvf(uint32_t fvf);
void mgfx_set_stream(IDirect3DVertexBuffer9* vb, uint32_t offset, uint32_t stride);
void mgfx_set_indices(IDirect3DIndexBuffer9* ib);
long mgfx_draw(uint32_t prim, int base_vertex, uint32_t start_index, uint32_t prim_count, const void* verts, uint32_t stride, uint32_t vcount, const void* indices, uint32_t icount, bool index32);
void mgfx_texture_upload(IDirect3DTexture9* tex);
long mgfx_texture_from_memory(const void* data, uint32_t size, IDirect3DTexture9** out);
constexpr uint32_t D3D_SDK_VERSION = 32;
constexpr long D3D_OK = 0;
constexpr long D3DERR_DEVICELOST = 0x88760868;
constexpr long D3DERR_INVALIDCALL = 0x8876086c;
constexpr long D3DERR_NOTAVAILABLE = 0x8876086a;
constexpr long D3DERR_OUTOFVIDEOMEMORY = 0x8876017c;
constexpr long D3DERR_DEVICENOTRESET = 0x88760869;
constexpr uint32_t D3DSGR_NO_CALIBRATION = 0;
using D3DDEVTYPE = uint32_t;
constexpr uint32_t D3DRS_ZFUNC = 23;
constexpr long S_OK = 0;
constexpr long E_FAIL = 0x80004005;
#define FAILED(hr) ((hr) < 0)
#define SUCCEEDED(hr) ((hr) >= 0)

constexpr uint32_t D3DADAPTER_DEFAULT = 0;
constexpr uint32_t D3DCREATE_HARDWARE_VERTEXPROCESSING = 0x40;
constexpr uint32_t D3DCREATE_MULTITHREADED = 0x4;
constexpr uint32_t D3DCLEAR_TARGET = 1;
constexpr uint32_t D3DCLEAR_ZBUFFER = 2;
constexpr uint32_t D3DCLEAR_STENCIL = 4;
constexpr uint32_t D3DPT_TRIANGLELIST = 4;
constexpr uint32_t D3DPT_TRIANGLESTRIP = 5;
constexpr uint32_t D3DPT_LINELIST = 2;
constexpr uint32_t D3DPT_LINESTRIP = 3;
constexpr uint32_t D3DPT_POINTLIST = 1;
constexpr uint32_t D3DFVF_XYZ = 0x002;
constexpr uint32_t D3DFVF_XYZRHW = 0x004;
constexpr uint32_t D3DFVF_NORMAL = 0x010;
constexpr uint32_t D3DFVF_DIFFUSE = 0x040;
constexpr uint32_t D3DFVF_TEX1 = 0x100;
constexpr uint32_t D3DPOOL_DEFAULT = 0;
constexpr uint32_t D3DPOOL_MANAGED = 1;
constexpr uint32_t D3DPOOL_SYSTEMMEM = 2;
constexpr uint32_t D3DUSAGE_RENDERTARGET = 1;
constexpr uint32_t D3DUSAGE_DEPTHSTENCIL = 2;
constexpr uint32_t D3DUSAGE_DYNAMIC = 0x200;
constexpr uint32_t D3DUSAGE_WRITEONLY = 8;
constexpr uint32_t D3DLOCK_DISCARD = 0x2000;
constexpr uint32_t D3DLOCK_READONLY = 0x10;
constexpr uint32_t D3DCAPS2_CANCALIBRATEGAMMA = 0x20000;
constexpr uint32_t D3DPRESENT_INTERVAL_ONE = 1;
constexpr uint32_t D3DPRESENT_INTERVAL_IMMEDIATE = 0x80000000;
constexpr uint32_t D3DSWAPEFFECT_FLIP = 1;
constexpr uint32_t D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL = 2;
constexpr uint32_t D3DDEVTYPE_HAL = 1;
constexpr uint32_t D3DFMT_UNKNOWN = 0;
constexpr uint32_t D3DFMT_R5G6B5 = 23;
constexpr uint32_t D3DFMT_R8G8B8 = 20;
constexpr uint32_t D3DFMT_A8R8G8B8 = 21;
constexpr uint32_t D3DFMT_X8R8G8B8 = 22;
constexpr uint32_t D3DFMT_D24S8 = 75;
constexpr uint32_t D3DFMT_D16 = 80;
constexpr uint32_t D3DTSS_COLOROP = 1;
constexpr uint32_t D3DTSS_COLORARG1 = 2;
constexpr uint32_t D3DTSS_COLORARG2 = 3;
constexpr uint32_t D3DTSS_ALPHAOP = 4;
constexpr uint32_t D3DTSS_ALPHAARG1 = 5;
constexpr uint32_t D3DTSS_ALPHAARG2 = 6;
constexpr uint32_t D3DTOP_DISABLE = 1;
constexpr uint32_t D3DTA_CURRENT = 1;
constexpr uint32_t D3DRS_ZENABLE = 7;
constexpr uint32_t D3DRS_LIGHTING = 137;
constexpr uint32_t D3DZB_TRUE = 1;

struct D3DDISPLAYMODE { uint32_t Width, Height, RefreshRate, Format; };
extern "C" void mg_desktop_size(int* w, int* h);
struct D3DPRESENT_PARAMETERS {
    uint32_t BackBufferWidth, BackBufferHeight, BackBufferFormat, BackBufferCount;
    uint32_t MultiSampleType, MultiSampleQuality, SwapEffect, hDeviceWindow;
    int Windowed, EnableAutoDepthStencil;
    uint32_t AutoDepthStencilFormat, Flags;
    uint32_t FullScreen_RefreshRateInHz, PresentationInterval;
};
struct D3DVIEWPORT9 { uint32_t X, Y, Width, Height; float MinZ, MaxZ; };
struct D3DCOLORVALUE { float r, g, b, a; };
struct D3DMATERIAL9 { D3DCOLORVALUE Diffuse, Ambient, Specular, Emissive; float Power; };
struct D3DVECTOR { float x, y, z; };
struct D3DLIGHT9 { uint32_t Type; D3DCOLORVALUE Diffuse, Specular, Ambient; D3DVECTOR Position, Direction; float Range, Falloff, Attenuation0, Attenuation1, Attenuation2, Theta, Phi; };
struct D3DGAMMARAMP { uint16_t red[256], green[256], blue[256]; };
struct D3DCAPS9 { uint32_t Caps2; uint32_t MaxTextureWidth; uint32_t MaxTextureHeight; uint32_t MaxSimultaneousTextures; uint32_t MaxStreams; uint32_t StencilCaps; uint32_t RasterCaps; uint32_t MaxAnisotropy; };
struct D3DRECT { int x1, y1, x2, y2; };
struct D3DLOCKED_RECT { int Pitch; void* pBits; };

struct IDirect3DSurface9;
void mgfx_set_render_target(IDirect3DSurface9*);
void mgfx_set_viewport(const D3DVIEWPORT9*);
void mgfx_get_viewport(D3DVIEWPORT9*);
struct IDirect3DTexture9;
void mgfx_track_texture(IDirect3DTexture9* tex, int alive);
struct IDirect3DSurface9 {
    uint32_t width = 1, height = 1, format = 21;
    bool shadow_target = false;
    IDirect3DTexture9* owner = nullptr;
    std::vector<uint8_t> pixels;
    std::vector<float> depth;
    std::vector<float> depth_keep;
    virtual ~IDirect3DSurface9() = default;
    virtual uint32_t Release() { return 0; }
    virtual long LockRect(D3DLOCKED_RECT* rect, const RECT*, uint32_t) {
        if (pixels.empty()) pixels.assign(size_t(width) * height * 4, 0);
        if (rect) { rect->Pitch = int(width * 4); rect->pBits = pixels.data(); }
        return S_OK;
    }
    virtual long UnlockRect() { return S_OK; }
    virtual long GetDesc(void* desc) {
        if (desc) { auto* d = static_cast<uint32_t*>(desc); d[0]=format; d[1]=1; d[2]=0; d[3]=0; d[4]=0; d[5]=0; d[6]=width; d[7]=height; }
        return S_OK;
    }
};
struct IDirect3DVertexBuffer9 {
    std::vector<uint8_t> bytes;
    virtual ~IDirect3DVertexBuffer9() = default;
    virtual uint32_t Release() { return 0; }
    virtual long Lock(uint32_t offset, uint32_t, void** data, uint32_t) { if (data) *data = bytes.data() + offset; return S_OK; }
    virtual long Unlock() { return S_OK; }
};
struct IDirect3DIndexBuffer9 {
    std::vector<uint8_t> bytes;
    bool index32 = false;
    virtual ~IDirect3DIndexBuffer9() = default;
    virtual uint32_t Release() { return 0; }
    virtual long Lock(uint32_t offset, uint32_t, void** data, uint32_t) { if (data) *data = bytes.data() + offset; return S_OK; }
    virtual long Unlock() { return S_OK; }
};
struct IDirect3DTexture9 {
    IDirect3DTexture9() { mgfx_track_texture(this, 1); }
    uint16_t bgfx_tex = 0xFFFF;
    uint16_t fb = 0xFFFF;
    uint32_t width = 1, height = 1;
    uint32_t format = 21;
    bool is_rt = false;
    int rt_view = -1;
    int rt_frame = -1;
    IDirect3DSurface9* surface = nullptr;
    std::vector<uint8_t> rgba;
    virtual ~IDirect3DTexture9() { mgfx_track_texture(this, 0); }
    virtual uint32_t Release() { return 0; }
    virtual uint32_t GetLevelCount() { return 1; }
    virtual long GetSurfaceLevel(uint32_t, IDirect3DSurface9** surface_out) {
        if (surface_out) {
            if (!surface) {
                surface = new IDirect3DSurface9();
                surface->width = width;
                surface->height = height;
                surface->shadow_target = true;
                surface->owner = this;
            }
            *surface_out = surface;
        }
        return S_OK;
    }
    bool lock_readonly = false;
    bool lock_a8 = false;
    std::vector<uint8_t> a8;
    virtual long LockRect(uint32_t, D3DLOCKED_RECT* rect, const RECT* area, uint32_t flags) {
        // shadow silhouettes are A8, one byte of alpha per texel
        const bool a8fmt = format == 28;
        lock_a8 = a8fmt;
        lock_readonly = (flags & 0x10) != 0;
        if (a8fmt) {
            if (a8.size() < size_t(width) * height) a8.assign(size_t(width) * height, 0);
        } else if (rgba.empty()) {
            rgba.resize(size_t(width) * height * 4);
        }
        if (rect) {
            uint32_t x = 0, y = 0;
            if (area) {
                if (area->left > 0) x = uint32_t(area->left);
                if (area->top > 0) y = uint32_t(area->top);
                if (x >= width) x = width - 1;
                if (y >= height) y = height - 1;
            }
            if (a8fmt) {
                rect->Pitch = int(width);
                rect->pBits = a8.data() + size_t(y) * width + x;
            } else {
                rect->Pitch = int(width * 4);
                rect->pBits = rgba.data() + (size_t(y) * width + x) * 4;
            }
        }
        return S_OK;
    }
    virtual long UnlockRect(uint32_t) {
        if (lock_a8) {
            // expand each alpha byte to white RGB so TFACTOR tint survives
            rgba.resize(size_t(width) * height * 4);
            for (uint32_t i = 0; i < width * height; ++i) {
                uint8_t a = i < a8.size() ? a8[i] : 0;
                uint8_t* p = rgba.data() + size_t(i) * 4;
                p[0] = p[1] = p[2] = 255;
                p[3] = a;
            }
            lock_a8 = false;
        }
        if (!lock_readonly) mgfx_texture_upload(this);
        lock_readonly = false;
        return S_OK;
    }
    virtual long GetLevelDesc(uint32_t, void* desc) { if (desc) { auto* d = static_cast<uint32_t*>(desc); d[0]=format?format:21; d[1]=0; d[2]=0; d[3]=0; d[4]=0; d[5]=0; d[6]=width?width:1; d[7]=height?height:1; } return S_OK; }
    virtual void PreLoad() {}
    virtual long AddDirtyRect(const D3DRECT*) { return S_OK; }
    virtual uint32_t SetPriority(uint32_t) { return 0; }
};
struct D3DDEVICE_CREATION_PARAMETERS { uint32_t AdapterOrdinal; uint32_t DeviceType; HWND hFocusWindow; uint32_t BehaviorFlags; };
using _D3DMULTISAMPLE_TYPE = uint32_t;
struct IDirect3DDevice9 {
    IDirect3DSurface9* m_depth = nullptr;
    virtual ~IDirect3DDevice9() = default;
    virtual uint32_t Release() { return 0; }
    virtual long Present(const void*, const void*, HWND, const void*) { return mgfx_present(); }
    virtual long BeginScene() { return mgfx_begin_scene(); }
    virtual long EndScene() { return S_OK; }
    virtual long Clear(uint32_t count, const D3DRECT*, uint32_t flags, D3DCOLOR color, float z, uint32_t) {
        // A non-zero rect count is a partial clear (HUD). bgfx can only clear the whole view,
        // and those calls use z=0, which would reject the whole map
        if (count) return S_OK;
        return mgfx_clear(flags, color, z);
    }
    virtual long GetRenderState(uint32_t, uint32_t* value) { if (value) *value = 0; return S_OK; }
    virtual long GetSamplerState(uint32_t stage, uint32_t type, uint32_t* value) {
        if (value) *value = mgfx_get_sampler(stage, type);
        return S_OK;
    }
    virtual long GetLight(uint32_t, D3DLIGHT9*) { return S_OK; }
    virtual long GetLightEnable(uint32_t, int* value) { if (value) *value = 0; return S_OK; }
    virtual long GetMaterial(D3DMATERIAL9*) { return S_OK; }
    virtual long SetRenderState(uint32_t state, uint32_t value) { mgfx_set_render_state(state, value); return S_OK; }
    virtual long SetTextureStageState(uint32_t stage, uint32_t type, uint32_t value) { mgfx_set_tss(stage, type, value); return S_OK; }
    virtual long SetSamplerState(uint32_t stage, uint32_t type, uint32_t value) { mgfx_set_sampler(stage, type, value); return S_OK; }
    virtual long SetTexture(uint32_t stage, IDirect3DTexture9* tex) { mgfx_set_texture(stage, tex); return S_OK; }
    virtual long SetTransform(uint32_t which, const void* matrix) { mgfx_set_transform(which, matrix); return S_OK; }
    virtual long SetFVF(uint32_t fvf) { mgfx_set_fvf(fvf); return S_OK; }
    virtual long SetStreamSource(uint32_t, IDirect3DVertexBuffer9* vb, uint32_t offset, uint32_t stride) { mgfx_set_stream(vb, offset, stride); return S_OK; }
    virtual long SetIndices(IDirect3DIndexBuffer9* ib) { mgfx_set_indices(ib); return S_OK; }
    virtual long DrawIndexedPrimitive(uint32_t prim, int base, uint32_t, uint32_t, uint32_t start, uint32_t prim_count) { return mgfx_draw(prim, base, start, prim_count, nullptr, 0, 0, nullptr, 0xFFFFFFFFu, false); }
    virtual long DrawPrimitive(uint32_t prim, uint32_t start, uint32_t prim_count) { return mgfx_draw(prim, int(start), 0, prim_count, nullptr, 0, 0, nullptr, 0, false); }
    virtual long DrawPrimitiveUP(uint32_t prim, uint32_t prim_count, const void* verts, uint32_t stride) { return mgfx_draw(prim, 0, 0, prim_count, verts, stride, 0, nullptr, 0, false); }
    virtual long DrawIndexedPrimitiveUP(uint32_t prim, uint32_t, uint32_t vcount, uint32_t prim_count, const void* indices, uint32_t index_stride, const void* verts, uint32_t stride) { return mgfx_draw(prim, 0, 0, prim_count, verts, stride, vcount, indices, 0, index_stride == 4); }
    virtual long CreateTexture(uint32_t w, uint32_t h, uint32_t, uint32_t usage, uint32_t fmt, uint32_t, IDirect3DTexture9** tex, void*) {
        if (tex) {
            *tex = new IDirect3DTexture9();
            (*tex)->width = w ? w : 1;
            (*tex)->height = h ? h : 1;
            (*tex)->format = fmt ? fmt : 21;
            (*tex)->is_rt = (usage & D3DUSAGE_RENDERTARGET) != 0;
            if (!(*tex)->is_rt) (*tex)->rgba.assign(size_t((*tex)->width) * (*tex)->height * 4, 0);
        }
        return S_OK;
    }
    virtual long CreateVertexBuffer(uint32_t bytes, uint32_t, uint32_t, uint32_t, IDirect3DVertexBuffer9** vb, void*) { if (vb) { *vb = new IDirect3DVertexBuffer9(); (*vb)->bytes.resize(bytes); } return S_OK; }
    virtual long CreateIndexBuffer(uint32_t bytes, uint32_t, uint32_t fmt, uint32_t, IDirect3DIndexBuffer9** ib, void*) { if (ib) { *ib = new IDirect3DIndexBuffer9(); (*ib)->bytes.resize(bytes); (*ib)->index32 = fmt == 102; } return S_OK; }
    virtual long CreateRenderTarget(uint32_t w, uint32_t h, uint32_t fmt, uint32_t, uint32_t, int, IDirect3DSurface9** out, void*) { if (out) { auto* s = new IDirect3DSurface9(); s->width = w?w:1; s->height = h?h:1; s->format = fmt; s->shadow_target = true; s->pixels.assign(s->width*s->height*4, 0); *out = s; } return S_OK; }
    virtual long CreateDepthStencilSurface(uint32_t w, uint32_t h, uint32_t fmt, uint32_t, uint32_t, int, IDirect3DSurface9** out, void*) { if (out) { auto* s = new IDirect3DSurface9(); s->width = w?w:1; s->height = h?h:1; s->format = fmt; *out = s; } return S_OK; }
    virtual long SetRenderTarget(uint32_t, IDirect3DSurface9* surface) { mgfx_set_render_target(surface); return S_OK; }
    virtual long GetRenderTarget(uint32_t, IDirect3DSurface9** surface) { if (surface) *surface = new IDirect3DSurface9(); return S_OK; }
    virtual long SetDepthStencilSurface(IDirect3DSurface9*) { return S_OK; }
    virtual long GetDepthStencilSurface(IDirect3DSurface9** out) { if (out) { if (!m_depth) { m_depth = new IDirect3DSurface9(); m_depth->width = 1024; m_depth->height = 768; } *out = m_depth; } return S_OK; }
    virtual long SetViewport(const D3DVIEWPORT9* vp) { mgfx_set_viewport(vp); return S_OK; }
    virtual long GetViewport(D3DVIEWPORT9* vp) { mgfx_get_viewport(vp); return S_OK; }
    virtual long SetMaterial(const D3DMATERIAL9*) { return S_OK; }
    virtual long SetLight(uint32_t, const D3DLIGHT9*) { return S_OK; }
    virtual long LightEnable(uint32_t, int) { return S_OK; }
    virtual long SetGammaRamp(uint32_t, uint32_t, const D3DGAMMARAMP*) { return S_OK; }
    virtual long GetGammaRamp(uint32_t, D3DGAMMARAMP*) { return S_OK; }
    virtual long GetDeviceCaps(D3DCAPS9* caps) { if (caps) { std::memset(caps, 0, sizeof(*caps)); caps->MaxTextureWidth = 4096; caps->MaxTextureHeight = 4096; caps->MaxSimultaneousTextures = 8; caps->MaxStreams = 8; caps->MaxAnisotropy = 16; caps->RasterCaps = 0x00100000; } return S_OK; }
    virtual uint32_t GetAvailableTextureMem() { return 256 * 1024 * 1024; }
    virtual long ColorFill(IDirect3DSurface9*, const D3DRECT*, D3DCOLOR) { return S_OK; }
    virtual long TestCooperativeLevel() { return S_OK; }
    virtual long Reset(D3DPRESENT_PARAMETERS* pp) {
        if (pp) mgfx_apply_present(pp->BackBufferWidth, pp->BackBufferHeight, pp->MultiSampleType, pp->PresentationInterval);
        return S_OK;
    }
    virtual long UpdateTexture(IDirect3DTexture9* src, IDirect3DTexture9* dst) {
        if (!src || !dst || src->rgba.empty()) return S_OK;
        dst->width = src->width ? src->width : 1;
        dst->height = src->height ? src->height : 1;
        dst->format = src->format ? src->format : 21;
        if (src->format == 28 && src->a8.size() >= size_t(dst->width) * dst->height) {
            dst->rgba.resize(size_t(dst->width) * dst->height * 4);
            for (uint32_t i = 0; i < dst->width * dst->height; ++i) {
                uint8_t* p = dst->rgba.data() + size_t(i) * 4;
                p[0] = p[1] = p[2] = 255;
                p[3] = src->a8[i];
            }
        } else {
            dst->rgba = src->rgba;
        }
        // A rendertarget flag makes the upload a no-op. Cache pixels never
        // reached the minimap, so the widget stayed at the clear color
        if (dst->is_rt && dst->bgfx_tex == 0xFFFF) {
            dst->is_rt = false;
            dst->fb = 0xFFFF;
        }
        mgfx_texture_upload(dst);
        return S_OK;
    }
    virtual long GetRenderTargetData(IDirect3DSurface9*, IDirect3DSurface9*) { return S_OK; }
    virtual int ShowCursor(int) { return 1; }
    virtual long GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* params) { if (params) std::memset(params, 0, sizeof(*params)); return S_OK; }
    virtual long GetFrontBufferData(uint32_t, IDirect3DSurface9*) { return S_OK; }
    virtual long CreateOffscreenPlainSurface(uint32_t w, uint32_t h, uint32_t fmt, uint32_t, IDirect3DSurface9** out, void*) {
        if (!out) return E_FAIL;
        auto* s = new IDirect3DSurface9();
        s->width = w ? w : 1;
        s->height = h ? h : 1;
        s->format = fmt;
        s->pixels.assign(size_t(s->width) * s->height * 4, 0);
        *out = s;
        return S_OK;
    }
    virtual long GetDisplayMode(uint32_t, D3DDISPLAYMODE* mode) {
        int w = 1280, h = 720;
        mg_desktop_size(&w, &h);
        if (mode) { mode->Width = uint32_t(w); mode->Height = uint32_t(h); mode->RefreshRate = 60; mode->Format = D3DFMT_X8R8G8B8; }
        return S_OK;
    }
    virtual long SetVertexShaderConstantF(uint32_t, const float*, uint32_t) { return S_OK; }
    virtual long SetPixelShaderConstantF(uint32_t, const float*, uint32_t) { return S_OK; }
    virtual long SetVertexShader(void*) { return S_OK; }
    virtual long SetPixelShader(void*) { return S_OK; }
};
struct IDirect3D9 {
    virtual ~IDirect3D9() = default;
    virtual uint32_t Release() { return 0; }
    virtual long GetAdapterDisplayMode(uint32_t, D3DDISPLAYMODE* mode) {
        int w = 1280, h = 720;
        mg_desktop_size(&w, &h);
        if (mode) { mode->Width = uint32_t(w); mode->Height = uint32_t(h); mode->RefreshRate = 60; mode->Format = D3DFMT_X8R8G8B8; }
        return S_OK;
    }
    virtual long CreateDevice(uint32_t, uint32_t, HWND, uint32_t, D3DPRESENT_PARAMETERS*, IDirect3DDevice9** device);
    virtual long CheckDeviceFormat(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) { return S_OK; }
    virtual long CheckDeviceMultiSampleType(uint32_t, uint32_t, uint32_t, int, uint32_t samples, DWORD*) {
        // bgfx can do these counts and no others
        switch (samples) {
            case 0: case 2: case 4: case 8: case 16: return S_OK;
            default: return E_FAIL;
        }
    }
    // The host calls this before any device exists: MatrixGameDll Support() uses it to
    virtual long GetDeviceCaps(uint32_t, uint32_t, D3DCAPS9* caps) {
        if (caps) {
            std::memset(caps, 0, sizeof(*caps));
            caps->MaxTextureWidth = 4096;
            caps->MaxTextureHeight = 4096;
            caps->MaxSimultaneousTextures = 8;
            caps->MaxStreams = 8;
        }
        return S_OK;
    }
};
IDirect3D9* Direct3DCreate9(uint32_t);
using D3DFORMAT = uint32_t;
using D3DTEXTUREOP = uint32_t;
struct D3DMATRIX {
    union {
        float m[4][4];
        struct { float _11,_12,_13,_14,_21,_22,_23,_24,_31,_32,_33,_34,_41,_42,_43,_44; };
    };
};
using LPDIRECT3DTEXTURE9 = IDirect3DTexture9*;
using LPDIRECT3DDEVICE9 = IDirect3DDevice9*;
using LPDIRECT3DVERTEXBUFFER9 = IDirect3DVertexBuffer9*;
using LPDIRECT3DINDEXBUFFER9 = IDirect3DIndexBuffer9*;
using LPDIRECT3DSURFACE9 = IDirect3DSurface9*;
struct D3DSURFACE_DESC { uint32_t Format, Type, Usage, Pool, MultiSampleType, MultiSampleQuality; uint32_t Width, Height; };
constexpr uint32_t D3DFVF_TEX2 = 0x200;
constexpr uint32_t D3DFVF_XYZB1 = 0x006;
constexpr uint32_t D3DFMT_INDEX16 = 101;
constexpr uint32_t D3DFMT_INDEX32 = 102;
constexpr uint32_t D3DTOP_SELECTARG1 = 2;
constexpr uint32_t D3DTOP_MODULATE = 4;
constexpr uint32_t D3DTOP_ADD = 7;
constexpr uint32_t D3DTA_TEXTURE = 2;
constexpr uint32_t D3DTA_DIFFUSE = 0;
constexpr uint32_t D3DTA_TFACTOR = 3;
constexpr uint32_t D3DRS_ZWRITEENABLE = 14;
constexpr uint32_t D3DRS_ALPHABLENDENABLE = 27;
constexpr uint32_t D3DRS_SRCBLEND = 19;
constexpr uint32_t D3DRS_DESTBLEND = 20;
constexpr uint32_t D3DRS_CULLMODE = 22;
constexpr uint32_t D3DRS_TEXTUREFACTOR = 60;
constexpr uint32_t D3DRS_ALPHATESTENABLE = 41;
constexpr uint32_t D3DCULL_NONE = 1;
constexpr uint32_t D3DCULL_CW = 2;
constexpr uint32_t D3DCULL_CCW = 3;
constexpr uint32_t D3DBLEND_SRCALPHA = 5;
constexpr uint32_t D3DBLEND_INVSRCALPHA = 6;
constexpr uint32_t D3DTS_WORLD = 256;
constexpr uint32_t D3DTS_VIEW = 2;
constexpr uint32_t D3DTS_PROJECTION = 3;
constexpr uint32_t D3DTS_TEXTURE0 = 16;
constexpr uint32_t D3DTS_TEXTURE1 = 17;
constexpr uint32_t D3DTS_TEXTURE2 = 18;
constexpr uint32_t D3DTS_TEXTURE3 = 19;
constexpr uint32_t D3DTS_TEXTURE4 = 20;
constexpr uint32_t D3DTS_TEXTURE5 = 21;
constexpr uint32_t D3DTS_TEXTURE6 = 22;
constexpr uint32_t D3DTS_TEXTURE7 = 23;
constexpr uint32_t D3DSAMP_MAGFILTER = 5;
constexpr uint32_t D3DSAMP_MINFILTER = 6;
constexpr uint32_t D3DSAMP_MIPFILTER = 7;
constexpr uint32_t D3DSAMP_ADDRESSU = 1;
constexpr uint32_t D3DSAMP_ADDRESSV = 2;
using D3DSAMPLERSTATETYPE = uint32_t;
using D3DTRANSFORMSTATETYPE = uint32_t;
using D3DRENDERSTATETYPE = uint32_t;
constexpr uint32_t D3DTEXF_POINT = 1;
constexpr uint32_t D3DTEXF_LINEAR = 2;
constexpr uint32_t D3DTEXF_ANISOTROPIC = 3;
constexpr uint32_t D3DSAMP_MAXANISOTROPY = 10;
constexpr uint32_t D3DTADDRESS_WRAP = 1;
constexpr uint32_t D3DTADDRESS_CLAMP = 3;
constexpr uint32_t D3DTSS_TEXCOORDINDEX = 11;
constexpr uint32_t D3DTSS_TEXTURETRANSFORMFLAGS = 24;
constexpr uint32_t D3DTSS_RESULTARG = 28;
constexpr uint32_t D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR = 0x00040000;
constexpr uint32_t D3DTOP_BLENDTEXTUREALPHA = 13;
constexpr uint32_t D3DTA_TEMP = 5;
constexpr uint32_t D3DSAMP_MIPMAPLODBIAS = 8;
constexpr uint32_t D3DRS_AMBIENT = 139;
constexpr uint32_t D3DTEXF_NONE = 0;
constexpr uint32_t D3DBLEND_ONE = 2;
constexpr uint32_t D3DBLEND_ZERO = 1;
using D3DPOOL = uint32_t;
using FLOAT = float;
#define D3DCOLOR_XRGB(r, g, b) (0xff000000u | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b))
#define D3DCOLOR_ARGB(a, r, g, b) ((uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b))
using LPDWORD = DWORD*;

constexpr uint32_t D3DRS_ADAPTIVETESS = 1000;
constexpr uint32_t D3DRS_AMBIENTMATERIALSOURCE = 1001;
constexpr uint32_t D3DRS_ANTIALIASEDLINEENABLE = 1002;
constexpr uint32_t D3DRS_BLENDFACTOR = 1003;
constexpr uint32_t D3DRS_BLENDOP = 1004;
constexpr uint32_t D3DRS_BLENDOPALPHA = 1005;
constexpr uint32_t D3DRS_CLIPPING = 1006;
constexpr uint32_t D3DRS_CLIPPLANEENABLE = 1007;
constexpr uint32_t D3DRS_COLORVERTEX = 1008;
constexpr uint32_t D3DRS_COLORWRITEENABLE1 = 1009;
constexpr uint32_t D3DRS_COLORWRITEENABLE2 = 1010;
constexpr uint32_t D3DRS_COLORWRITEENABLE3 = 1011;
constexpr uint32_t D3DRS_DEBUGMONITORTOKEN = 1012;
constexpr uint32_t D3DRS_DEPTHBIAS = 1013;
constexpr uint32_t D3DRS_DESTBLENDALPHA = 1014;
constexpr uint32_t D3DRS_DIFFUSEMATERIALSOURCE = 1015;
constexpr uint32_t D3DRS_DITHERENABLE = 1016;
constexpr uint32_t D3DRS_EMISSIVEMATERIALSOURCE = 1017;
constexpr uint32_t D3DRS_ENABLEADAPTIVETESSELLATION = 1018;
constexpr uint32_t D3DRS_FOGDENSITY = 1019;
constexpr uint32_t D3DRS_INDEXEDVERTEXBLENDENABLE = 1020;
constexpr uint32_t D3DRS_LASTPIXEL = 1021;
constexpr uint32_t D3DRS_LOCALVIEWER = 1022;
constexpr uint32_t D3DRS_MAXTESSELLATIONLEVEL = 1023;
constexpr uint32_t D3DRS_MINTESSELLATIONLEVEL = 1024;
constexpr uint32_t D3DRS_MULTISAMPLEMASK = 1025;
constexpr uint32_t D3DRS_NORMALDEGREE = 1026;
constexpr uint32_t D3DRS_PATCHEDGESTYLE = 1027;
constexpr uint32_t D3DRS_POINTSCALE = 1028;
constexpr uint32_t D3DRS_POINTSCALEENABLE = 1029;
constexpr uint32_t D3DRS_POINTSIZE = 1030;
constexpr uint32_t D3DRS_POINTSPRITEENABLE = 1031;
constexpr uint32_t D3DRS_POSITIONDEGREE = 1032;
constexpr uint32_t D3DRS_RANGEFOGENABLE = 1033;
constexpr uint32_t D3DRS_SCISSORTESTENABLE = 1034;
constexpr uint32_t D3DRS_SEPARATEALPHABLENDENABLE = 1035;
constexpr uint32_t D3DRS_SLOPESCALEDEPTHBIAS = 1036;
constexpr uint32_t D3DRS_SPECULARENABLE = 1037;
constexpr uint32_t D3DRS_SRCBLENDALPHA = 1038;
constexpr uint32_t D3DRS_SRGBWRITEENABLE = 1039;
constexpr uint32_t D3DRS_TWEENFACTOR = 1040;
constexpr uint32_t D3DRS_VERTEXBLEND = 1041;
constexpr uint32_t D3DRS_WRAP0 = 1042;
constexpr uint32_t D3DRS_WRAP1 = 1043;
constexpr uint32_t D3DRS_WRAP2 = 1044;
constexpr uint32_t D3DRS_WRAP3 = 1045;
constexpr uint32_t D3DRS_WRAP4 = 1046;
constexpr uint32_t D3DRS_WRAP5 = 1047;
constexpr uint32_t D3DRS_WRAP6 = 1048;
constexpr uint32_t D3DRS_WRAP7 = 1049;
constexpr uint32_t D3DRS_WRAP8 = 1050;
constexpr uint32_t D3DRS_WRAP9 = 1051;
constexpr uint32_t D3DRS_WRAP10 = 1052;
constexpr uint32_t D3DRS_WRAP11 = 1053;
constexpr uint32_t D3DRS_WRAP12 = 1054;
constexpr uint32_t D3DRS_WRAP13 = 1055;
constexpr uint32_t D3DRS_WRAP14 = 1056;
constexpr uint32_t D3DRS_WRAP15 = 1057;
constexpr uint32_t D3DRS_ADAPTIVETESS_W = 2000;
constexpr uint32_t D3DRS_ADAPTIVETESS_X = 2001;
constexpr uint32_t D3DRS_ADAPTIVETESS_Y = 2002;
constexpr uint32_t D3DRS_ADAPTIVETESS_Z = 2003;
constexpr uint32_t D3DRS_ALPHAFUNC = 2004;
constexpr uint32_t D3DRS_ALPHAREF = 2005;
constexpr uint32_t D3DRS_CCW_STENCILFAIL = 2006;
constexpr uint32_t D3DRS_CCW_STENCILFUNC = 2007;
constexpr uint32_t D3DRS_CCW_STENCILPASS = 2008;
constexpr uint32_t D3DRS_CCW_STENCILZFAIL = 2009;
constexpr uint32_t D3DRS_FILLMODE = 2010;
constexpr uint32_t D3DRS_FOGCOLOR = 2011;
constexpr uint32_t D3DRS_FOGENABLE = 2012;
constexpr uint32_t D3DRS_FOGEND = 2013;
constexpr uint32_t D3DRS_FOGSTART = 2014;
constexpr uint32_t D3DRS_FOGTABLEMODE = 2015;
constexpr uint32_t D3DRS_FOGVERTEXMODE = 2016;
constexpr uint32_t D3DRS_MULTISAMPLEANTIALIAS = 2017;
constexpr uint32_t D3DRS_NORMALIZENORMALS = 2018;
constexpr uint32_t D3DRS_POINTSCALE_A = 2019;
constexpr uint32_t D3DRS_POINTSCALE_B = 2020;
constexpr uint32_t D3DRS_POINTSCALE_C = 2021;
constexpr uint32_t D3DRS_POINTSIZE_MAX = 2022;
constexpr uint32_t D3DRS_POINTSIZE_MIN = 2023;
constexpr uint32_t D3DRS_SHADEMODE = 2024;
constexpr uint32_t D3DRS_SPECULARMATERIALSOURCE = 2025;
constexpr uint32_t D3DRS_STENCILENABLE = 2026;
constexpr uint32_t D3DRS_STENCILFAIL = 2027;
constexpr uint32_t D3DRS_STENCILFUNC = 2028;
constexpr uint32_t D3DRS_STENCILMASK = 2029;
constexpr uint32_t D3DRS_STENCILPASS = 2030;
constexpr uint32_t D3DRS_STENCILREF = 2031;
constexpr uint32_t D3DRS_STENCILWRITEMASK = 2032;
constexpr uint32_t D3DRS_STENCILZFAIL = 2033;
constexpr uint32_t D3DRS_COLORWRITEENABLE = 168;
constexpr uint32_t D3DFMT_A4R4G4B4 = 26;
constexpr uint32_t D3DFMT_X1R5G5B5 = 24;
constexpr uint32_t D3DFMT_L8 = 50;
constexpr uint32_t D3DFMT_A8 = 28;
constexpr uint32_t D3DRS_TWOSIDEDSTENCILMODE = 2034;
constexpr uint32_t D3DSAMP_ADDRESSW = 3000;
constexpr uint32_t D3DSAMP_BORDERCOLOR = 3001;
constexpr uint32_t D3DSAMP_DMAPOFFSET = 3002;
constexpr uint32_t D3DSAMP_ELEMENTINDEX = 3003;
constexpr uint32_t D3DSAMP_MAXMIPLEVEL = 3004;
constexpr uint32_t D3DSAMP_SRGBTEXTURE = 3005;

#define D3DFVF_TEXTUREFORMAT1 3
#define D3DFVF_TEXTUREFORMAT2 0
#define D3DFVF_TEXTUREFORMAT3 1
#define D3DFVF_TEXTUREFORMAT4 2
#define D3DFVF_TEXCOORDSIZE1(i) (D3DFVF_TEXTUREFORMAT1 << ((i)*2 + 16))
#define D3DFVF_TEXCOORDSIZE2(i) (D3DFVF_TEXTUREFORMAT2 << ((i)*2 + 16))
#define D3DFVF_TEXCOORDSIZE3(i) (D3DFVF_TEXTUREFORMAT3 << ((i)*2 + 16))
#define D3DFVF_TEXCOORDSIZE4(i) (D3DFVF_TEXTUREFORMAT4 << ((i)*2 + 16))
constexpr uint32_t D3DTTFF_COUNT1 = 1;
constexpr uint32_t D3DTTFF_COUNT3 = 3;
constexpr uint32_t D3DTTFF_COUNT4 = 4;
constexpr uint32_t D3DTTFF_PROJECTED = 256;

constexpr uint32_t D3DTTFF_DISABLE = 0;
constexpr uint32_t D3DTTFF_COUNT2 = 2;
constexpr uint32_t D3DBLEND_SRCCOLOR = 3;
constexpr uint32_t D3DBLEND_INVSRCCOLOR = 4;
constexpr uint32_t D3DBLEND_DESTALPHA = 7;
constexpr uint32_t D3DBLEND_INVDESTALPHA = 8;
constexpr uint32_t D3DBLEND_DESTCOLOR = 9;
constexpr uint32_t D3DBLEND_INVDESTCOLOR = 10;
constexpr uint32_t D3DTA_SPECULAR = 4;
constexpr uint32_t D3DTA_COMPLEMENT = 16;
constexpr uint32_t D3DTA_ALPHAREPLICATE = 32;
constexpr uint32_t D3DTOP_SELECTARG2 = 3;
constexpr uint32_t D3DTOP_MODULATE2X = 5;
constexpr uint32_t D3DTOP_MODULATE4X = 6;
constexpr uint32_t D3DTOP_ADDSIGNED = 8;
constexpr uint32_t D3DTOP_ADDSIGNED2X = 9;
constexpr uint32_t D3DTOP_SUBTRACT = 10;
constexpr uint32_t D3DTOP_ADDSMOOTH = 11;
constexpr uint32_t D3DTOP_BLENDDIFFUSEALPHA = 12;
constexpr uint32_t D3DTOP_BLENDFACTORALPHA = 14;
constexpr uint32_t D3DTOP_BLENDTEXTUREALPHAPM = 15;
constexpr uint32_t D3DTOP_BLENDCURRENTALPHA = 16;
constexpr uint32_t D3DTSS_BUMPENVMAT00 = 7;
constexpr uint32_t D3DTSS_TCI_PASSTHRU = 0;
constexpr uint32_t D3DCMP_NEVER = 1;
constexpr uint32_t D3DCMP_LESS = 2;
constexpr uint32_t D3DCMP_EQUAL = 3;
constexpr uint32_t D3DCMP_LESSEQUAL = 4;
constexpr uint32_t D3DCMP_GREATER = 5;
constexpr uint32_t D3DCMP_NOTEQUAL = 6;
constexpr uint32_t D3DCMP_GREATEREQUAL = 7;
constexpr uint32_t D3DCMP_ALWAYS = 8;
constexpr uint32_t D3DFILL_SOLID = 0;
constexpr uint32_t D3DFILL_WIREFRAME = 0;
constexpr uint32_t D3DFMT_A8L8 = 0;
constexpr uint32_t D3DFMT_A8R3G3B2 = 0;
constexpr uint32_t D3DFMT_D24X8 = 0;
constexpr uint32_t D3DFMT_DXT1 = 0;
constexpr uint32_t D3DFOG_LINEAR = 0;
constexpr uint32_t D3DFOG_NONE = 0;
constexpr uint32_t D3DLIGHT_DIRECTIONAL = 0;
constexpr uint32_t D3DLOCK_NOOVERWRITE = 0;
constexpr uint32_t D3DMCS_MATERIAL = 0;
constexpr uint32_t D3DMULTISAMPLE_NONE = 0;
constexpr uint32_t D3DPRASTERCAPS_WFOG = 0x00100000;
constexpr uint32_t D3DPT_TRIANGLEFAN = 6;
constexpr uint32_t D3DSGR_CALIBRATE = 0;
constexpr uint32_t D3DSHADE_FLAT = 0;
constexpr uint32_t D3DSHADE_GOURAUD = 0;
constexpr uint32_t D3DSTENCILCAPS_TWOSIDED = 0;
constexpr uint32_t D3DSTENCILOP_KEEP = 1;
constexpr uint32_t D3DSTENCILOP_ZERO = 2;
constexpr uint32_t D3DSTENCILOP_REPLACE = 3;
constexpr uint32_t D3DSTENCILOP_INCRSAT = 4;
constexpr uint32_t D3DSTENCILOP_DECRSAT = 5;
constexpr uint32_t D3DSTENCILOP_INVERT = 6;
constexpr uint32_t D3DSTENCILOP_INCR = 7;
constexpr uint32_t D3DSTENCILOP_DECR = 8;
constexpr uint32_t D3DSWAPEFFECT_DISCARD = 0;
constexpr uint32_t D3DTSS_TCI_CAMERASPACENORMAL = 0;
constexpr uint32_t D3DZB_FALSE = 0;
constexpr uint32_t GWL_STYLE = 0;
constexpr uint32_t SW_MINIMIZE = 0;
constexpr uint32_t SW_SHOWNORMAL = 0;
constexpr uint32_t VK_ADD = 0x6B;
constexpr uint32_t VK_APPS = 0x5D;
constexpr uint32_t VK_BACK = 0x08;
constexpr uint32_t VK_CAPITAL = 0x14;
constexpr uint32_t VK_CONTROL = 0x11;
constexpr uint32_t VK_DELETE = 0x2E;
constexpr uint32_t VK_DIVIDE = 0x6F;
constexpr uint32_t VK_DOWN = 0x28;
constexpr uint32_t VK_END = 0x23;
constexpr uint32_t VK_F9 = 0x78;
constexpr uint32_t VK_HOME = 0x24;
constexpr uint32_t VK_INSERT = 0x2D;
constexpr uint32_t VK_LCONTROL = 0xA2;
constexpr uint32_t VK_LEFT = 0x25;
constexpr uint32_t VK_LMENU = 0xA4;
constexpr uint32_t VK_LSHIFT = 0xA0;
constexpr uint32_t VK_LWIN = 0x5B;
constexpr uint32_t VK_MENU = 0x12;
constexpr uint32_t VK_MULTIPLY = 0x6A;
constexpr uint32_t VK_NEXT = 0x22;
constexpr uint32_t VK_NUMLOCK = 0x90;
constexpr uint32_t VK_NUMPAD0 = 0x60;
constexpr uint32_t VK_NUMPAD1 = 0x61;
constexpr uint32_t VK_NUMPAD2 = 0x62;
constexpr uint32_t VK_NUMPAD3 = 0x63;
constexpr uint32_t VK_NUMPAD4 = 0x64;
constexpr uint32_t VK_NUMPAD5 = 0x65;
constexpr uint32_t VK_NUMPAD6 = 0x66;
constexpr uint32_t VK_NUMPAD7 = 0x67;
constexpr uint32_t VK_NUMPAD8 = 0x68;
constexpr uint32_t VK_NUMPAD9 = 0x69;
constexpr uint32_t VK_OEM_4 = 0xDB;
constexpr uint32_t VK_OEM_5 = 0xDC;
constexpr uint32_t VK_OEM_6 = 0xDD;
constexpr uint32_t VK_OEM_COMMA = 0xBC;
constexpr uint32_t VK_OEM_MINUS = 0xBD;
constexpr uint32_t VK_OEM_PLUS = 0xBB;
constexpr uint32_t VK_PAUSE = 0x13;
constexpr uint32_t VK_PRIOR = 0x21;
constexpr uint32_t VK_RCONTROL = 0xA3;
constexpr uint32_t VK_RIGHT = 0x27;
constexpr uint32_t VK_RMENU = 0xA5;
constexpr uint32_t VK_RSHIFT = 0xA1;
constexpr uint32_t VK_RWIN = 0x5C;
constexpr uint32_t VK_SCROLL = 0x91;
constexpr uint32_t VK_SEPARATOR = 0x6C;
constexpr uint32_t VK_SUBTRACT = 0x6D;
constexpr uint32_t VK_TAB = 0x09;
constexpr uint32_t VK_UP = 0x26;
