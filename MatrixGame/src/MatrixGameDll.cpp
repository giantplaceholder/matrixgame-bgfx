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

#include <time.h>
#include <windows.h>

SMGDRobotInterface g_RobotInterface;
SMGDRangersInterface *g_RangersInterface = nullptr;
int g_ExitState = 0;

long __stdcall ExceptionHandler(PEXCEPTION_POINTERS) {
    return EXCEPTION_EXECUTE_HANDLER;
}

BOOL APIENTRY DllMain(
    [[maybe_unused]] HANDLE hModule,
    DWORD ul_reason_for_call,
    [[maybe_unused]] LPVOID lpReserved)
{
    SetUnhandledExceptionFilter(ExceptionHandler);
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

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
    uint32_t seed = (unsigned)time(nullptr);

    CGame::Init(hinst, hwnd, map, seed, settings, lang, txt_start, txt_win, txt_loss, planet);
    
    CFormMatrixGame formgame;

    CGame::RunGameLoop(&formgame);

    CGame::SaveResult(rgs);
    CGame::SafeFree();

    ClipCursor(nullptr);

    if (FLAG(g_Flags, GFLAG_EXITLOOP))
        return g_ExitState;
    else
        return 0;
}

MATRIXGAMEDLL_API SMGDRobotInterface *__cdecl GetRobotInterface(void) {
    ZeroMemory(&g_RobotInterface, sizeof(SMGDRobotInterface));
    g_RobotInterface.m_Init = &Init;
    g_RobotInterface.m_Deinit = &Deinit;
    g_RobotInterface.m_Support = &Support;
    g_RobotInterface.m_Run = &Run;
    return &g_RobotInterface;
}
