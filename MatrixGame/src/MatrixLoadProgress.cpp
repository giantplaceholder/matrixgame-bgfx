// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "MatrixLoadProgress.hpp"
#include "MatrixGameDll.hpp"
#include "Math3D.hpp"

static SLoadProcessProps lp_props[] = {
        {L"Preloading robots", 70},
        {L"Loading map", 300},
        {L"Preparing objects", 150},
        {L"Preparing Interface", 30},

};

CLoadProgress::CLoadProgress(void) {
    m_CurLoadProcess = -1;
    const int n = sizeof(lp_props) / sizeof(SLoadProcessProps);
    m_fullsize = 0;
    for (int i = 0; i < n; ++i) {
        m_fullsize += lp_props[i].len;
    }

    m_cursizedone = 0;
    m_fullsize1 = 1.0f / float(m_fullsize);

    m_lastacc = 0;
}

void CLoadProgress::SetCurLP(int lp) {
    if (m_CurLoadProcess >= 0) {
        m_cursizedone += lp_props[m_CurLoadProcess].len;
    };
    m_CurLoadProcess = lp;

#ifdef _DEBUG
    OutputDebugStringW(utils::format(L"\n%ls", lp_props[lp].description).c_str());
#endif
}
void CLoadProgress::SetCurLPPos(int i) {
    float k = m_fullsize1 * (m_cursizedone + ((m_cur_lp_size1 * float(i)) * lp_props[m_CurLoadProcess].len));

    int ac = Float2Int(k * LPACCURACY);
    if (ac > m_lastacc) {
        if (g_RangersInterface)
            g_RangersInterface->m_ProgressBar(float(ac) / LPACCURACY);

#ifdef _DEBUG
        OutputDebugStringW(utils::format(L"\n%d", ac).c_str());
#endif

        m_lastacc = ac;
    }
}