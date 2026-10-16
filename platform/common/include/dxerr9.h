// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once
inline const char* DXGetErrorString9(long) { return "d3d"; }
inline const wchar_t* DXGetErrorString9W(long) { return L"d3d"; }
inline const wchar_t* DXGetErrorDescription9W(long) { return L"d3d"; }
