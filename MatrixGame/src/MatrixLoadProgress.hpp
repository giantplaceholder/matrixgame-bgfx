// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#ifndef MATRIX_LOAD_PROGRESS_INCLUDE
#define MATRIX_LOAD_PROGRESS_INCLUDE

#include "CMain.hpp"

#define LPACCURACY 100

#define LP_PRELOADROBOTS     0
#define LP_LOADINGMAP        1
#define LP_PREPARINGOBJECTS  2
#define LP_PREPARININTERFACE 3

struct SLoadProcessProps {
    const wchar_t *description;
    int len;
};

class CLoadProgress : public CMain {
    int m_CurLoadProcess;
    int m_fullsize;
    float m_fullsize1;
    int m_cursizedone;
    int m_cur_lp_size;
    float m_cur_lp_size1;

    int m_lastacc;

public:
    CLoadProgress(void);
    ~CLoadProgress(void) {}

    void SetCurLP(int lp);
    void InitCurLP(int sz) {
        m_cur_lp_size = sz;
        m_cur_lp_size1 = 1.0f / float(sz);
    };
    void SetCurLPPos(int i);
};

#endif