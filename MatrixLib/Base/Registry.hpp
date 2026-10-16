// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include "BaseDef.hpp"

#include <string>
#include <windows.h>

namespace Base {

void Reg_GetString(HKEY pkey, const wchar_t *path, const wchar_t *name, const std::wstring &str);
std::wstring Reg_GetString(HKEY pkey, const wchar_t *path, const wchar_t *name, const wchar_t *defaultstr);
void Reg_SetString(HKEY pkey, const wchar_t *path, const wchar_t *name, const wchar_t *str);

}  // namespace Base
