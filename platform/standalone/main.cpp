// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "windows.h"

int APIENTRY WinMain(HINSTANCE, HINSTANCE, wchar_t*, int);
#if !defined(_WIN32)
#include <execinfo.h>
#include <csignal>
#include <cstdio>
#include <unistd.h>

static void on_crash(int sig) {
    void* frames[32];
    int n = backtrace(frames, 32);
    dprintf(2, "signal %d\n", sig);
    backtrace_symbols_fd(frames, n, 2);
    _exit(128 + sig);
}
#endif

int main(int argc, char** argv) {
#if !defined(_WIN32)
    signal(SIGSEGV, on_crash);
    signal(SIGABRT, on_crash);
#endif
    mg_set_command_line(argc, argv);
    return WinMain(nullptr, nullptr, GetCommandLineW(), 1);
}
