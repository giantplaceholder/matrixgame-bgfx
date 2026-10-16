// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include <cstdint>

namespace FilePNG
{

// format: 1-gray 2-rgb 3-rgba 4-palate
uintptr_t ReadStart_Buf(void *soubuf, uint32_t soubuflen, uint32_t *lenx, uint32_t *leny, uint32_t *countcolor, uint32_t *format);
uint32_t Read(uintptr_t id, void *buf, uint32_t lenline, uint32_t *arraycolor);

// Возвращает полный размер файла. Если больше bufoutlen то нужно вызвать повторно. При ошибке 0
int Write(void *bufout, int bufoutlen, void *buf, uint32_t ll, uint32_t lx, uint32_t ly, uint32_t bytepp,
                                     int rgb_to_bgr);

} // namespace FilePNG