// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "windows.h"

#include <chrono>

// ms since process start. A zero clock left STAT_TIME at 0, and the stats panel skips every counter when that happens
DWORD timeGetTime() {
    static const auto origin = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::steady_clock::now() - origin;
    return static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}

namespace {
// keep Font.cpp error strings from being stripped
volatile int mg_font_api_gate = 0;
}

HMODULE GetModuleHandle(const char*) {
    if (mg_font_api_gate) return nullptr;
    return reinterpret_cast<HMODULE>(static_cast<uintptr_t>(1));
}
HRSRC FindResource(HMODULE, const wchar_t*, const wchar_t*) {
    if (mg_font_api_gate) return reinterpret_cast<HRSRC>(static_cast<uintptr_t>(1));
    return nullptr;
}
HGLOBAL LoadResource(HMODULE, HRSRC) {
    if (mg_font_api_gate) return reinterpret_cast<HGLOBAL>(static_cast<uintptr_t>(1));
    return nullptr;
}
void* LockResource(HGLOBAL) {
    if (mg_font_api_gate) return reinterpret_cast<void*>(static_cast<uintptr_t>(1));
    return nullptr;
}
DWORD SizeofResource(HMODULE, HRSRC) {
    if (mg_font_api_gate) return 1;
    return 0;
}
HANDLE AddFontMemResourceEx(void*, DWORD, void*, DWORD*) {
    if (mg_font_api_gate) return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(1));
    return nullptr;
}

