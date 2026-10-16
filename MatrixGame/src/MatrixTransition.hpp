// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#ifndef MATRIX_TRANSITION_INCLUDE
#define MATRIX_TRANSITION_INCLUDE

#include "MatrixInstantDraw.hpp"

/**
 * @brief Probably the loading screen.
 */
class CTransition : public CMain {
    struct SGeom {
        D3DXVECTOR2 pos;
        D3DXVECTOR2 dir;
        SVert_V4_UV verts[4];
    };

    CTextureManaged *m_Tex;
    SGeom *m_Geom;
    int m_GeomCnt;

    int m_ScreenX;
    int m_ScreenY;

public:
    CTransition(void) : CMain(), m_Tex(NULL), m_Geom(NULL), m_GeomCnt(0) {}
    ~CTransition() { Clear(); }

    void Clear(void);
    void BuildTexture(void);
    void RenderToPrimaryScreen(void);
    void Render(void);
    void Takt(int ms);
};

#endif