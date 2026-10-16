// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#ifndef MATRIX_SAMPLER_INCLUDE
#define MATRIX_SAMPLER_INCLUDE

struct SRobotsSettings;

class CMatrixSampleStateManager {
public:
    int m_AFDegree;

    void ApplySettings(SRobotsSettings *set);
    HRESULT SetState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value);

    CMatrixSampleStateManager(void);
    ~CMatrixSampleStateManager(void);

private:
    bool EligibleForAnisotropy(D3DSAMPLERSTATETYPE type);
};

extern CMatrixSampleStateManager g_Sampler;

#endif
