// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once
#include "windows.h"
struct IDirectDrawSurface7 { virtual uint32_t Release() { return 0; } };
struct DDPIXELFORMAT {
    uint32_t dwSize = 0, dwFlags = 0, dwFourCC = 0, dwRGBBitCount = 0;
    uint32_t dwRBitMask = 0, dwGBitMask = 0, dwBBitMask = 0, dwRGBAlphaBitMask = 0;
};
struct DDSCAPS2 { uint32_t dwCaps = 0, dwCaps2 = 0, dwCaps3 = 0, dwCaps4 = 0; };
struct DDSURFACEDESC2 {
    uint32_t dwSize = 0;
    uint32_t dwFlags = 0;
    uint32_t dwHeight = 0;
    uint32_t dwWidth = 0;
    union { int32_t lPitch; uint32_t dwLinearSize; };
    uint32_t dwBackBufferCount = 0;
    uint32_t dwMipMapCount = 0;
    uint32_t dwAlphaBitDepth = 0;
    uint32_t dwReserved = 0;
    void* lpSurface = nullptr;
    DDPIXELFORMAT ddpfPixelFormat{};
    DDSCAPS2 ddsCaps{};
};
constexpr uint32_t DDSD_CAPS = 0x1;
constexpr uint32_t DDSD_HEIGHT = 0x2;
constexpr uint32_t DDSD_WIDTH = 0x4;
constexpr uint32_t DDSD_PITCH = 0x8;
constexpr uint32_t DDSD_PIXELFORMAT = 0x1000;
constexpr uint32_t DDSD_LPSURFACE = 0x800;
constexpr uint32_t DDSD_LINEARSIZE = 0x80000;
constexpr uint32_t DDPF_RGB = 0x40;
constexpr uint32_t DDPF_ALPHAPIXELS = 0x1;
constexpr uint32_t DDSCAPS_TEXTURE = 0x1000;
