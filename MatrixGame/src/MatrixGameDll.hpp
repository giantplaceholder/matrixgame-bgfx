// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include <BaseDef.hpp>
#include <windows.h>
#include <cstdint>
#include <string>

enum ESupportError {
    SUPE_OK = 0,
    SUPE_DIRECTX = 1,        // non 9.x DirectX
    SUPE_VIDEODRIVER = 2,    // driver too old
    SUPE_VIDEOHARDWARE = 3,  // video hardware unsupported

};

struct SRobotsSettings {
    // pointer-sized, a 32-bit long would shift the rest of the struct
    intptr_t FDirect3D;
    intptr_t FD3DDevice;
    // можно менять во время игры
    bool m_ShowStencilShadows;
    bool m_ShowProjShadows;
    bool m_IzvratMS;  // извратный selection объектов

    // нужен рестарт
    bool m_LandTexturesGloss;
    bool m_ObjTexturesGloss;
    bool m_SoftwareCursor;
    byte m_SkyBox;       // 0 - none, 1 - dds (low quality), 2 - png (high quality)
    byte m_RobotShadow;  // 0 - none, 1 - stencil

    int m_BPP;  // 16, 32
    int m_ResolutionX;
    int m_ResolutionY;
    int m_RefreshRate;  // 0-default

    float m_Brightness;
    float m_Contrast;

    int m_FSAASamples;
    int m_AFDegree;
    float m_MaxDistance;
    bool m_VSync;

    /**
     *  Explicit constructor for SRobotsSettings object.
     *
     *  Used inside EXE build as default settings.
     *
     *  Note: I am afraid that adding a "real" constructor could break compatibility with
     *  Space Rangers because they could be relying on the implicit default "zero value" constructor
     *  which would be overridden.
     */
    static SRobotsSettings generate_default_settings()
    {
        SRobotsSettings settings;

        settings.m_ShowStencilShadows = 1;
        settings.m_ShowProjShadows = 1;
        settings.m_IzvratMS = 0;
        settings.m_LandTexturesGloss = 1;
        settings.m_ObjTexturesGloss = 1;
        settings.m_SoftwareCursor = 0;
        settings.m_SkyBox = 2;
        settings.m_RobotShadow = 1;
        settings.m_BPP = 32;
        // settings.m_ResolutionX = screen_width; // unfortunatelly can't set this up now without reading .dat file
        // settings.m_ResolutionY = screen_height;
        settings.m_RefreshRate = 0;
        settings.m_Brightness = 0.5;
        settings.m_Contrast = 0.5;
        settings.m_FSAASamples = 0;
        settings.m_AFDegree = 0;
        settings.m_MaxDistance = 1;
        settings.m_VSync = 1;

        return settings;
    }
};

struct SRobotGameState {
    int m_Time;
    int m_BuildRobot;
    int m_KillRobot;
    int m_BuildTurret;
    int m_KillTurret;
    int m_KillBuilding;
};

struct SMGDRobotInterface;
struct SMGDRangersInterface;
struct SMGDRangersInterfaceText;

struct SMGDRobotInterface {
    void(__stdcall *m_Init)(SMGDRangersInterface *ri);
    void(__stdcall *m_Deinit)();
    int(__stdcall *m_Support)();
    int(__stdcall *m_Run)(HINSTANCE hinst, HWND hwnd, wchar *map, SRobotsSettings *set, wchar *lang, wchar *txt_start,
                          wchar *txt_win, wchar *txt_loss, wchar *planet, SRobotGameState *rgs);
};

struct SMGDRangersInterface {
    void(__stdcall *m_Sound)(wchar *path);

    // opaque host handle, kept pointer-sized so it is not truncated
    uintptr_t(__stdcall *m_SoundCreate)(wchar *path, int group, int loop);
    void(__stdcall *m_SoundDestroy)(uintptr_t id);
    void(__stdcall *m_SoundPlay)(uintptr_t id);
    int(__stdcall *m_SoundIsPlay)(uintptr_t id);
    void(__stdcall *m_SoundVolume)(uintptr_t id, float vol);
    void(__stdcall *m_SoundPan)(uintptr_t id, float pan);
    float(__stdcall *m_SoundGetVolume)(uintptr_t id);
    float(__stdcall *m_SoundGetPan)(uintptr_t id);

    void(__stdcall *m_RangersText)(wchar *text, wchar *font, DWORD color, int sizex, int sizey, int alignx, int aligny,
                                   int wordwrap, int smex, int smy, Base::CRect *clipr, SMGDRangersInterfaceText *it);
    void(__stdcall *m_RangersTextClear)(SMGDRangersInterfaceText *it);

    void(__stdcall *m_ProgressBar)(float val);

    void(__stdcall *m_Takt)(void);

    void(__stdcall *m_Begin)(void);

    float(__stdcall *m_MusicVolumeGet)(void);
    void(__stdcall *m_MusicVolumeSet)(float val);
};

struct SMGDRangersInterfaceText {
    DWORD m_Image;
    BYTE *m_Buf;
    DWORD m_Pitch;
    int m_SizeX;
    int m_SizeY;
};

#ifdef MATRIXGAME_EXPORTS
#define MATRIXGAMEDLL_API __declspec(dllexport)
#else
#define MATRIXGAMEDLL_API __declspec(dllimport)
#endif

extern "C" {
MATRIXGAMEDLL_API SMGDRobotInterface *__cdecl GetRobotInterface(void);
}

extern SMGDRobotInterface g_RobotInterface;
extern SMGDRangersInterface *g_RangersInterface;

// host text is wchar_t, do not recode it to UTF-16 on Linux

// 0-exit to windows (terminate)
extern int g_ExitState;
