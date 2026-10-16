// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once
// MG_TRACE=1 writes matrixgame-trace.log
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

inline bool mg_trace_enabled() {
    static const bool on = [] {
        const char* value = std::getenv("MG_TRACE");
        if (!value || !value[0] || std::strcmp(value, "0") == 0) return false;
        const char* path = std::getenv("MG_TRACE_FILE");
        if (!path || !path[0]) path = "matrixgame-trace.log";
        if (std::freopen(path, "w", stdout)) std::setvbuf(stdout, nullptr, _IONBF, 0);
        if (std::freopen(path, "a", stderr)) std::setvbuf(stderr, nullptr, _IONBF, 0);
        std::fprintf(stdout, "[mg] trace -> %s\n", path);
        std::fflush(stdout);
        return true;
    }();
    return on;
}

inline void mg_trace(const char* fmt, ...) {
    if (!mg_trace_enabled()) return;
    char line[512];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    std::fputs(line, stdout);
    std::fflush(stdout);
}
