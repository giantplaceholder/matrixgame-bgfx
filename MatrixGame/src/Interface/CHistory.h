// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "CConstructor.h"

class CHistory : public CMain {
    SRobotConfig *m_CurrentConfig;

    void LoadCurrentConfigToConstructor();
    void DelConfig(SRobotConfig *config);
    void FindConfig(SRobotConfig *config);

public:
    SRobotConfig *m_FirstConfig;
    SRobotConfig *m_LastConfig;

    void AddConfig(SRobotConfig *config);
    void __stdcall PrevConfig(void *object);
    void __stdcall NextConfig(void *object);

    bool IsNext() {
        if (m_CurrentConfig && m_CurrentConfig->m_NextConfig)
            return true;
        else
            return false;
    }
    bool IsPrev() {
        if (m_CurrentConfig && m_CurrentConfig->m_PrevConfig)
            return true;
        else
            return false;
    }

    CHistory();
    ~CHistory();
};