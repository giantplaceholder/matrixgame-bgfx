// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include <cstdint>
#include <cstring>

inline void memcopy_back_dword(void *tgt, const void *src,
                                      uint32_t size)  // callers pass a count of pointers (a dword on 32-bit)
{
    if (!tgt || !src || !size) return;
    std::memmove(tgt, src, size_t(size) * sizeof(void *));
}
