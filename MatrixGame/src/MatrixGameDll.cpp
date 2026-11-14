// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "MatrixGameDll.hpp"
#include "MatrixFormGame.hpp"
#include "MatrixGame.h"
#include "3g.hpp"
#include "MatrixCrashLog.hpp"

#include <time.h>
#include <exception>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <windows.h>

SMGDRobotInterface g_RobotInterface;
SMGDRangersInterface *g_RangersInterface = nullptr;
int g_ExitState = 0;

#if defined(_WIN32)
static void *g_vectored_exception_handle = nullptr;

LONG __stdcall MgFirstChanceExceptionHandler(PEXCEPTION_POINTERS info) {
    MgLogWindowsFirstChanceException(info);
    return EXCEPTION_CONTINUE_SEARCH; // observe only, let the host handle it
}
#endif

LONG __stdcall ExceptionHandler(PEXCEPTION_POINTERS info) {
    char buf[128];
    unsigned code = 0;
    // read the exception code without dereferencing the record
    if (info && info->ExceptionRecord)
        std::memcpy(&code, info->ExceptionRecord, sizeof(code));
    std::snprintf(buf, sizeof(buf), "unhandled SEH exception code=0x%08X", code);
    // keep the exception path independent of game-object state
    MgLogCrashWrite(buf, false);
    return EXCEPTION_EXECUTE_HANDLER;
}

[[noreturn]] void MgTerminateHandler() noexcept {
    const char *reason = "std::terminate invoked";
    try {
        if (auto ep = std::current_exception())
            std::rethrow_exception(ep);
    } catch (const std::exception &e) {
        MgLogCrashWrite(e.what(), false);
        std::abort();
    } catch (...) {
        MgLogCrashWrite("std::terminate invoked for non-standard exception", false);
        std::abort();
    }
    MgLogCrashWrite(reason, false);
    std::abort();
}

BOOL APIENTRY DllMain(
    [[maybe_unused]] HANDLE hModule,
    DWORD ul_reason_for_call,
    [[maybe_unused]] LPVOID lpReserved)
{
    SetUnhandledExceptionFilter(ExceptionHandler);
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
#if defined(_WIN32)
            MgLogSetWindowsModuleBase(reinterpret_cast<uintptr_t>(hModule));
#endif
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
#if defined(_WIN32)
            if (g_vectored_exception_handle) {
                RemoveVectoredExceptionHandler(g_vectored_exception_handle);
                g_vectored_exception_handle = nullptr;
            }
#endif
            break;
    }
    return TRUE;
}

// leave fatal POSIX signals to the OS

void __stdcall Init(SMGDRangersInterface *ri) {
    g_RangersInterface = ri;
}

void __stdcall Deinit() {
    g_RangersInterface = NULL;
}

int __stdcall Support() {
    // g_D3D = Direct3DCreate9(D3D_SDK_VERSION);
    g_D3D = Direct3DCreate9(31);

    if (g_D3D == NULL)
        return SUPE_DIRECTX;

    D3DCAPS9 caps;
    if (D3D_OK != g_D3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps)) {
        g_D3D->Release();
        g_D3D = NULL;
        return SUPE_DIRECTX;
    }

    if (caps.MaxSimultaneousTextures < 2)
        return SUPE_VIDEOHARDWARE;

    if (caps.MaxTextureWidth < 2048)
        return SUPE_VIDEOHARDWARE;
    if (caps.MaxTextureHeight < 2048)
        return SUPE_VIDEOHARDWARE;

    if (caps.MaxStreams == 0)
        return SUPE_VIDEODRIVER;
    g_D3D->Release();
    g_D3D = NULL;
    return SUPE_OK;
}

int __stdcall Run(HINSTANCE hinst, HWND hwnd, wchar *map, SRobotsSettings *settings, wchar *lang, wchar *txt_start,
                  wchar *txt_win, wchar *txt_loss, wchar *planet, SRobotGameState *rgs) {
    // reinstall the filter when the battle entry point is entered
    SetUnhandledExceptionFilter(ExceptionHandler);
    std::set_terminate(MgTerminateHandler);

    const uint32_t seed = (unsigned)time(nullptr);
    MgLogInitWrite("battle init starting");
#if defined(_WIN32)
    // catch VEH access violations before host frame-based SEH handlers
    if (!g_vectored_exception_handle)
        g_vectored_exception_handle = AddVectoredExceptionHandler(1, MgFirstChanceExceptionHandler);
    MgLogSetWindowsVehInstalled(g_vectored_exception_handle != nullptr);
    MgLogInitWrite(g_vectored_exception_handle ? "battle init starting; Windows VEH registered"
                                               : "ERROR: Windows VEH registration failed");
#endif
    try {
        CGame::Init(hinst, hwnd, map, seed, settings, lang, txt_start, txt_win, txt_loss, planet);
        MgLogInitWrite("battle init done");

        CFormMatrixGame formgame;
        CGame::RunGameLoop(&formgame);
        CGame::SaveResult(rgs);
        CGame::SafeFree();
        ClipCursor(nullptr);

        if (FLAG(g_Flags, GFLAG_EXITLOOP))
            return g_ExitState;
        return 0;
    } catch (const std::exception &e) {
        MgLogCrashWrite(e.what(), false);
        throw;
    } catch (...) {
        MgLogCrashWrite("unknown exception escaped robot battle Run()", false);
        throw;
    }
}

MATRIXGAMEDLL_API SMGDRobotInterface *__cdecl GetRobotInterface(void) {
    ZeroMemory(&g_RobotInterface, sizeof(SMGDRobotInterface));
    g_RobotInterface.m_Init = &Init;
    g_RobotInterface.m_Deinit = &Deinit;
    g_RobotInterface.m_Support = &Support;
    g_RobotInterface.m_Run = &Run;
    return &g_RobotInterface;
}
