// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once
#define random posix_random
#include "d3d9.h"
#include <cmath>
#include <algorithm>
#undef random

constexpr float D3DX_PI = 3.14159265358979323846f;
constexpr uint32_t D3DX_FILTER_NONE = 1;
constexpr uint32_t D3DX_FILTER_BOX = 2;
constexpr uint32_t D3DX_FILTER_TRIANGLE = 4;
constexpr uint32_t D3DX_FILTER_DITHER = 8;
constexpr uint32_t D3DX_DEFAULT = 0xffffffffu;

struct D3DXVECTOR2 {
    float x, y;
    D3DXVECTOR2() : x(0), y(0) {}
    D3DXVECTOR2(float ax, float ay) : x(ax), y(ay) {}
    D3DXVECTOR2& operator*=(float s) { x *= s; y *= s; return *this; }
    D3DXVECTOR2 operator*(float s) const { return {x * s, y * s}; }
    D3DXVECTOR2 operator/(float s) const { return {x / s, y / s}; }
    D3DXVECTOR2 operator-(const D3DXVECTOR2& o) const { return {x - o.x, y - o.y}; }
    D3DXVECTOR2 operator+(const D3DXVECTOR2& o) const { return {x + o.x, y + o.y}; }
    D3DXVECTOR2& operator+=(const D3DXVECTOR2& o) { x += o.x; y += o.y; return *this; }
    D3DXVECTOR2& operator-=(const D3DXVECTOR2& o) { x -= o.x; y -= o.y; return *this; }
};
struct D3DXVECTOR3 {
    float x, y, z;
    D3DXVECTOR3() : x(0), y(0), z(0) {}
    D3DXVECTOR3(float ax, float ay, float az) : x(ax), y(ay), z(az) {}
    D3DXVECTOR3(const D3DVECTOR& v) : x(v.x), y(v.y), z(v.z) {}
    operator D3DVECTOR() const { return {x, y, z}; }
    D3DXVECTOR3 operator+(const D3DXVECTOR3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    D3DXVECTOR3 operator-(const D3DXVECTOR3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    D3DXVECTOR3 operator*(float s) const { return {x * s, y * s, z * s}; }
    D3DXVECTOR3 operator/(float s) const { return {x / s, y / s, z / s}; }
    D3DXVECTOR3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }
    D3DXVECTOR3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    D3DXVECTOR3& operator+=(const D3DXVECTOR3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    D3DXVECTOR3& operator-=(const D3DXVECTOR3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    D3DXVECTOR3 operator-() const { return {-x, -y, -z}; }
    bool operator!=(const D3DXVECTOR3& o) const { return x != o.x || y != o.y || z != o.z; }
    bool operator==(const D3DXVECTOR3& o) const { return x == o.x && y == o.y && z == o.z; }
};
inline D3DXVECTOR3 operator*(float s, const D3DXVECTOR3& v) { return {v.x * s, v.y * s, v.z * s}; }
inline D3DXVECTOR2 operator*(float s, const D3DXVECTOR2& v) { return v * s; }
struct D3DXVECTOR4 { float x, y, z, w; };
struct D3DXMATRIX {
    union {
        float m[4][4];
        struct { float _11,_12,_13,_14,_21,_22,_23,_24,_31,_32,_33,_34,_41,_42,_43,_44; };
    };
    D3DXMATRIX() { std::memset(this, 0, sizeof(*this)); }
    D3DXMATRIX(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l, float n, float o, float p, float q) {
        _11=a; _12=b; _13=c; _14=d; _21=e; _22=f; _23=g; _24=h; _31=i; _32=j; _33=k; _34=l; _41=n; _42=o; _43=p; _44=q;
    }
    operator D3DMATRIX() const { D3DMATRIX d{}; std::memcpy(d.m, m, sizeof(d.m)); return d; }
};
struct D3DXPLANE { float a, b, c, d; };
struct D3DXQUATERNION { float x, y, z, w; };

inline D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3* out, const D3DXVECTOR3* v) {
    float len = std::sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    if (len > 0) { out->x = v->x / len; out->y = v->y / len; out->z = v->z / len; }
    return out;
}
inline float D3DXVec3Length(const D3DXVECTOR3* v) { return std::sqrt(v->x * v->x + v->y * v->y + v->z * v->z); }
inline float D3DXVec3LengthSq(const D3DXVECTOR3* v) { return v->x * v->x + v->y * v->y + v->z * v->z; }
inline float D3DXVec3Dot(const D3DXVECTOR3* a, const D3DXVECTOR3* b) { return a->x * b->x + a->y * b->y + a->z * b->z; }
inline D3DXVECTOR3* D3DXVec3Cross(D3DXVECTOR3* out, const D3DXVECTOR3* a, const D3DXVECTOR3* b) {
    *out = {a->y * b->z - a->z * b->y, a->z * b->x - a->x * b->z, a->x * b->y - a->y * b->x};
    return out;
}
inline D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3* out, const D3DXVECTOR3* v, const D3DXMATRIX* m) {
    float x = v->x * m->m[0][0] + v->y * m->m[1][0] + v->z * m->m[2][0] + m->m[3][0];
    float y = v->x * m->m[0][1] + v->y * m->m[1][1] + v->z * m->m[2][1] + m->m[3][1];
    float z = v->x * m->m[0][2] + v->y * m->m[1][2] + v->z * m->m[2][2] + m->m[3][2];
    float w = v->x * m->m[0][3] + v->y * m->m[1][3] + v->z * m->m[2][3] + m->m[3][3];
    if (w != 0) { x /= w; y /= w; z /= w; }
    *out = {x, y, z};
    return out;
}
inline D3DXVECTOR3* D3DXVec3TransformNormal(D3DXVECTOR3* out, const D3DXVECTOR3* v, const D3DXMATRIX* m) {
    *out = {v->x * m->m[0][0] + v->y * m->m[1][0] + v->z * m->m[2][0],
            v->x * m->m[0][1] + v->y * m->m[1][1] + v->z * m->m[2][1],
            v->x * m->m[0][2] + v->y * m->m[1][2] + v->z * m->m[2][2]};
    return out;
}
inline D3DXVECTOR3* D3DXVec3TransformCoordArray(D3DXVECTOR3* out, uint32_t out_stride, const D3DXVECTOR3* in, uint32_t in_stride, const D3DXMATRIX* m, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        D3DXVec3TransformCoord(reinterpret_cast<D3DXVECTOR3*>(reinterpret_cast<char*>(out) + i * out_stride),
                               reinterpret_cast<const D3DXVECTOR3*>(reinterpret_cast<const char*>(in) + i * in_stride), m);
    }
    return out;
}
inline float D3DXVec2Length(const D3DXVECTOR2* v) { return std::sqrt(v->x * v->x + v->y * v->y); }
inline float D3DXVec2Dot(const D3DXVECTOR2* a, const D3DXVECTOR2* b) { return a->x * b->x + a->y * b->y; }
inline float D3DXVec2LengthSq(const D3DXVECTOR2* v) { return v->x * v->x + v->y * v->y; }
inline D3DXVECTOR2* D3DXVec2Normalize(D3DXVECTOR2* out, const D3DXVECTOR2* v) {
    float len = D3DXVec2Length(v);
    if (len > 0) { out->x = v->x / len; out->y = v->y / len; }
    return out;
}
inline D3DXVECTOR2* D3DXVec2TransformCoord(D3DXVECTOR2* out, const D3DXVECTOR2* v, const D3DXMATRIX* m) {
    D3DXVECTOR3 p{v->x, v->y, 0}, r;
    D3DXVec3TransformCoord(&r, &p, m);
    *out = {r.x, r.y};
    return out;
}
inline D3DVECTOR& assign_vec(D3DVECTOR& dst, const D3DXVECTOR3& v) { dst.x = v.x; dst.y = v.y; dst.z = v.z; return dst; }
inline D3DXMATRIX operator*(const D3DXMATRIX& a, const D3DXMATRIX& b) {
    D3DXMATRIX out{};
    for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c)
        for (int k = 0; k < 4; ++k) out.m[r][c] += a.m[r][k] * b.m[k][c];
    return out;
}
inline D3DXMATRIX& operator*=(D3DXMATRIX& a, const D3DXMATRIX& b) { a = a * b; return a; }
inline D3DXMATRIX* D3DXMatrixMultiply(D3DXMATRIX* out, const D3DXMATRIX* a, const D3DXMATRIX* b) { *out = (*a) * (*b); return out; }
inline D3DXMATRIX* D3DXMatrixIdentity(D3DXMATRIX* out) { std::memset(out, 0, sizeof(*out)); out->m[0][0] = out->m[1][1] = out->m[2][2] = out->m[3][3] = 1; return out; }
inline D3DXMATRIX* D3DXMatrixTranslation(D3DXMATRIX* out, float x, float y, float z) { D3DXMatrixIdentity(out); out->m[3][0] = x; out->m[3][1] = y; out->m[3][2] = z; return out; }
inline D3DXMATRIX* D3DXMatrixScaling(D3DXMATRIX* out, float x, float y, float z) { D3DXMatrixIdentity(out); out->m[0][0] = x; out->m[1][1] = y; out->m[2][2] = z; return out; }
inline D3DXMATRIX* D3DXMatrixRotationX(D3DXMATRIX* out, float a) { D3DXMatrixIdentity(out); out->m[1][1] = std::cos(a); out->m[1][2] = std::sin(a); out->m[2][1] = -std::sin(a); out->m[2][2] = std::cos(a); return out; }
inline D3DXMATRIX* D3DXMatrixRotationY(D3DXMATRIX* out, float a) { D3DXMatrixIdentity(out); out->m[0][0] = std::cos(a); out->m[0][2] = -std::sin(a); out->m[2][0] = std::sin(a); out->m[2][2] = std::cos(a); return out; }
inline D3DXMATRIX* D3DXMatrixRotationZ(D3DXMATRIX* out, float a) { D3DXMatrixIdentity(out); out->m[0][0] = std::cos(a); out->m[0][1] = std::sin(a); out->m[1][0] = -std::sin(a); out->m[1][1] = std::cos(a); return out; }
inline D3DXMATRIX* D3DXMatrixRotationYawPitchRoll(D3DXMATRIX* out, float y, float p, float r) { D3DXMATRIX a, b, c; D3DXMatrixRotationY(&a, y); D3DXMatrixRotationX(&b, p); D3DXMatrixRotationZ(&c, r); *out = c * b * a; return out; }
inline D3DXMATRIX* D3DXMatrixLookAtLH(D3DXMATRIX* out, const D3DXVECTOR3* eye, const D3DXVECTOR3* at, const D3DXVECTOR3* up) {
    D3DXVECTOR3 z{at->x - eye->x, at->y - eye->y, at->z - eye->z};
    float zl = std::sqrt(z.x*z.x + z.y*z.y + z.z*z.z); if (zl == 0) zl = 1; z.x /= zl; z.y /= zl; z.z /= zl;
    D3DXVECTOR3 x{up->y*z.z - up->z*z.y, up->z*z.x - up->x*z.z, up->x*z.y - up->y*z.x};
    float xl = std::sqrt(x.x*x.x + x.y*x.y + x.z*x.z); if (xl == 0) xl = 1; x.x /= xl; x.y /= xl; x.z /= xl;
    D3DXVECTOR3 y{z.y*x.z - z.z*x.y, z.z*x.x - z.x*x.z, z.x*x.y - z.y*x.x};
    D3DXMatrixIdentity(out);
    out->m[0][0] = x.x; out->m[1][0] = x.y; out->m[2][0] = x.z; out->m[3][0] = -(x.x*eye->x + x.y*eye->y + x.z*eye->z);
    out->m[0][1] = y.x; out->m[1][1] = y.y; out->m[2][1] = y.z; out->m[3][1] = -(y.x*eye->x + y.y*eye->y + y.z*eye->z);
    out->m[0][2] = z.x; out->m[1][2] = z.y; out->m[2][2] = z.z; out->m[3][2] = -(z.x*eye->x + z.y*eye->y + z.z*eye->z);
    return out;
}
inline D3DXMATRIX* D3DXMatrixPerspectiveFovLH(D3DXMATRIX* out, float fov, float aspect, float zn, float zf) {
    std::memset(out, 0, sizeof(*out));
    float y = 1.f / std::tan(fov * 0.5f);
    float x = aspect != 0 ? y / aspect : y;
    float d = zf - zn; if (d == 0) d = 1;
    out->m[0][0] = x; out->m[1][1] = y; out->m[2][2] = zf / d; out->m[2][3] = 1.f; out->m[3][2] = -zn * zf / d;
    return out;
}
inline D3DXMATRIX* D3DXMatrixOrthoLH(D3DXMATRIX* out, float w, float h, float zn, float zf) {
    std::memset(out, 0, sizeof(*out));
    float d = zf - zn; if (d == 0) d = 1; if (w == 0) w = 1; if (h == 0) h = 1;
    out->m[0][0] = 2.f / w; out->m[1][1] = 2.f / h; out->m[2][2] = 1.f / d; out->m[3][2] = -zn / d; out->m[3][3] = 1.f;
    return out;
}
inline long D3DXLoadSurfaceFromSurface(IDirect3DSurface9*, const void*, const void*, IDirect3DSurface9*, const void*, const void*, uint32_t, D3DCOLOR) { return S_OK; }
inline D3DXVECTOR2* D3DXVec2TransformCoordArray(D3DXVECTOR2* out, uint32_t out_stride, const D3DXVECTOR2* in, uint32_t in_stride, const D3DXMATRIX* m, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        D3DXVec2TransformCoord(reinterpret_cast<D3DXVECTOR2*>(reinterpret_cast<char*>(out) + i * out_stride), reinterpret_cast<const D3DXVECTOR2*>(reinterpret_cast<const char*>(in) + i * in_stride), m);
    }
    return out;
}
inline D3DXMATRIX* D3DXMatrixInverse(D3DXMATRIX* out, float* det, const D3DXMATRIX* in) {
    if (!in) return nullptr;
    float a[4][8];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) a[r][c] = in->m[r][c];
        for (int c = 0; c < 4; ++c) a[r][4 + c] = r == c ? 1.f : 0.f;
    }
    float sign = 1.f;
    for (int col = 0; col < 4; ++col) {
        int pivot = col;
        float best = std::fabs(a[col][col]);
        for (int r = col + 1; r < 4; ++r) {
            float v = std::fabs(a[r][col]);
            if (v > best) { best = v; pivot = r; }
        }
        if (best < 1e-12f) {
            if (det) *det = 0;
            if (out && out != in) D3DXMatrixIdentity(out);
            else if (out) D3DXMatrixIdentity(out);
            return nullptr;
        }
        if (pivot != col) {
            sign = -sign;
            for (int c = 0; c < 8; ++c) std::swap(a[col][c], a[pivot][c]);
        }
        float div = a[col][col];
        sign *= div;
        for (int c = 0; c < 8; ++c) a[col][c] /= div;
        for (int r = 0; r < 4; ++r) if (r != col) {
            float f = a[r][col];
            if (f == 0) continue;
            for (int c = 0; c < 8; ++c) a[r][c] -= f * a[col][c];
        }
    }
    if (det) *det = sign;
    if (out) {
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c) out->m[r][c] = a[r][4 + c];
    }
    return out;
}
inline D3DXPLANE* D3DXPlaneFromPoints(D3DXPLANE* out, const D3DXVECTOR3* a, const D3DXVECTOR3* b, const D3DXVECTOR3* c) {
    D3DXVECTOR3 ab = *b - *a;
    D3DXVECTOR3 ac = *c - *a;
    D3DXVECTOR3 n;
    D3DXVec3Cross(&n, &ab, &ac);
    float len = D3DXVec3Length(&n);
    if (len > 1e-12f) { n.x /= len; n.y /= len; n.z /= len; }
    else n = {0, 0, 1};
    out->a = n.x; out->b = n.y; out->c = n.z;
    out->d = -(n.x * a->x + n.y * a->y + n.z * a->z);
    return out;
}
inline D3DXVECTOR3* D3DXPlaneIntersectLine(D3DXVECTOR3* out, const D3DXPLANE* plane, const D3DXVECTOR3* a, const D3DXVECTOR3* b) {
    if (!out || !plane || !a || !b) return nullptr;
    float da = plane->a * a->x + plane->b * a->y + plane->c * a->z + plane->d;
    float db = plane->a * b->x + plane->b * b->y + plane->c * b->z + plane->d;
    float denom = da - db;
    if (std::fabs(denom) < 1e-12f) return nullptr;
    float t = da / denom;
    *out = {a->x + (b->x - a->x) * t, a->y + (b->y - a->y) * t, a->z + (b->z - a->z) * t};
    return out;
}
inline D3DXPLANE* D3DXPlaneFromPointNormal(D3DXPLANE* out, const D3DXVECTOR3* p, const D3DXVECTOR3* n) { *out = {n->x, n->y, n->z, -(n->x * p->x + n->y * p->y + n->z * p->z)}; return out; }
inline D3DXVECTOR3* D3DXVec3Project(D3DXVECTOR3* out, const D3DXVECTOR3* v, const D3DVIEWPORT9* vp, const D3DXMATRIX* proj, const D3DXMATRIX* view, const D3DXMATRIX* world) {
    D3DXMATRIX wvp;
    D3DXMatrixIdentity(&wvp);
    if (world) wvp = wvp * *world;
    if (view) wvp = wvp * *view;
    if (proj) wvp = wvp * *proj;
    D3DXVECTOR3 r;
    D3DXVec3TransformCoord(&r, v, &wvp);
    float sx = vp ? float(vp->X) : 0;
    float sy = vp ? float(vp->Y) : 0;
    float sw = vp && vp->Width ? float(vp->Width) : 2.f;
    float sh = vp && vp->Height ? float(vp->Height) : 2.f;
    float zn = vp ? vp->MinZ : 0.f;
    float zf = vp ? vp->MaxZ : 1.f;
    out->x = sx + (r.x + 1.f) * 0.5f * sw;
    out->y = sy + (1.f - r.y) * 0.5f * sh;
    out->z = zn + r.z * (zf - zn);
    return out;
}

int mgfx_draw_text(const wchar_t* text, int len, void* rect, uint32_t format, uint32_t color, int pixel_height);
void mgfx_draw_screen_rgba(int x, int y, int w, int h, const void* rgba, int pitch);
void mgfx_shutdown_device();
struct D3DXFONT_DESCW { wchar_t FaceName[64]; int Height; };
struct ID3DXFont {
    int height = 13;
    virtual ~ID3DXFont() = default;
    virtual uint32_t Release() { return 0; }
    virtual int DrawTextW(void*, const wchar_t* text, int len, void* rect, uint32_t format, D3DCOLOR color) {
        return mgfx_draw_text(text, len, rect, format, color, height);
    }
    virtual int DrawTextA(void*, const char*, int, void*, uint32_t, D3DCOLOR) { return 0; }
    virtual long OnResetDevice() { return S_OK; }
    virtual long OnLostDevice() { return S_OK; }
    virtual HDC GetDC() { return nullptr; }
    virtual long GetDescW(D3DXFONT_DESCW* desc) { if (desc) { std::memset(desc, 0, sizeof(*desc)); desc->Height = height; } return S_OK; }
};
long D3DXCreateFontW(IDirect3DDevice9*, int height, uint32_t, uint32_t, uint32_t, int, uint32_t, uint32_t, uint32_t, uint32_t, const wchar_t*, ID3DXFont** font);
inline int D3DXBoxBoundProbe(const D3DXVECTOR3* bmin, const D3DXVECTOR3* bmax,
                            const D3DXVECTOR3* orig, const D3DXVECTOR3* dir) {
    if (!bmin || !bmax || !orig || !dir)
        return 0;
    float tmin = -1.0e30f;
    float tmax = 1.0e30f;
    const float o[3] = {orig->x, orig->y, orig->z};
    const float d[3] = {dir->x, dir->y, dir->z};
    const float mn[3] = {bmin->x, bmin->y, bmin->z};
    const float mx[3] = {bmax->x, bmax->y, bmax->z};
    for (int a = 0; a < 3; ++a) {
        if (std::fabs(d[a]) < 1.0e-12f) {
            if (o[a] < mn[a] || o[a] > mx[a])
                return 0;
            continue;
        }
        float inv = 1.0f / d[a];
        float t0 = (mn[a] - o[a]) * inv;
        float t1 = (mx[a] - o[a]) * inv;
        if (t0 > t1) {
            float s = t0;
            t0 = t1;
            t1 = s;
        }
        if (t0 > tmin)
            tmin = t0;
        if (t1 < tmax)
            tmax = t1;
        if (tmin > tmax || tmax < 0.0f)
            return 0;
    }
    return 1;
}
inline long D3DXCreateTexture(IDirect3DDevice9*, uint32_t w, uint32_t h, uint32_t, uint32_t, uint32_t, uint32_t, IDirect3DTexture9** texture) {
    if (texture) {
        *texture = new IDirect3DTexture9();
        (*texture)->width = w ? w : 1;
        (*texture)->height = h ? h : 1;
        (*texture)->rgba.assign(size_t((*texture)->width) * (*texture)->height * 4, 0);
    }
    return S_OK;
}
inline long D3DXCreateTextureFromFileInMemoryEx(IDirect3DDevice9*, const void* data, uint32_t size, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t filter, uint32_t mip_filter, D3DCOLOR, void*, void*, IDirect3DTexture9** texture) {
    (void)filter;
    (void)mip_filter;
    return mgfx_texture_from_memory(data, size, texture);
}
struct ID3DXRenderToSurface {
    virtual ~ID3DXRenderToSurface() = default;
    virtual uint32_t Release() { return 0; }
    virtual long BeginScene(IDirect3DSurface9* surface, const void*) { mgfx_begin_cpu_target(surface); return S_OK; }
    virtual long EndScene(uint32_t) { mgfx_end_cpu_target(); return S_OK; }
};
inline long D3DXCreateRenderToSurface(IDirect3DDevice9*, uint32_t, uint32_t, uint32_t, int, uint32_t, ID3DXRenderToSurface** out) {
    if (out) *out = new ID3DXRenderToSurface();
    return S_OK;
}
using LPD3DXFONT = ID3DXFont*;
constexpr uint32_t DEFAULT_QUALITY = 0;
