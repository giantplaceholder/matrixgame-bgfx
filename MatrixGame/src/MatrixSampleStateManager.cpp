// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include <windows.h>
#include "d3d9types.h"
#include "MatrixSampleStateManager.hpp"
#include "MatrixGameDll.hpp"
#include "3g.hpp"

CMatrixSampleStateManager::CMatrixSampleStateManager(void) {}

CMatrixSampleStateManager::~CMatrixSampleStateManager(void) {}

void CMatrixSampleStateManager::ApplySettings(SRobotsSettings *set) {
    m_AFDegree = g_D3DDCaps.MaxAnisotropy < set->m_AFDegree ? g_D3DDCaps.MaxAnisotropy : set->m_AFDegree;
}

HRESULT CMatrixSampleStateManager::SetState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value) {
    if (EligibleForAnisotropy(type) && (m_AFDegree) && (value == D3DTEXF_LINEAR)) {
        HRESULT hr;
        hr = g_D3DD->SetSamplerState(sampler, type, D3DTEXF_ANISOTROPIC);
        if (D3D_OK != hr)
            return hr;
        return g_D3DD->SetSamplerState(sampler, D3DSAMP_MAXANISOTROPY, m_AFDegree);
    }
    return g_D3DD->SetSamplerState(sampler, type, value);
}

bool CMatrixSampleStateManager::EligibleForAnisotropy(D3DSAMPLERSTATETYPE type) {
    return (type == D3DSAMP_MIPFILTER) || (type == D3DSAMP_MAGFILTER) || (type == D3DSAMP_MINFILTER);
}

CMatrixSampleStateManager g_Sampler;