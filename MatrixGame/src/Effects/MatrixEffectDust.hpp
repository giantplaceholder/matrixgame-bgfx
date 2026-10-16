// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include "MatrixEffectBillboard.hpp"

#define DUST_BILLS_CNT 5

class CMatrixEffectDust : public CMatrixEffect {
    CMatrixEffectBillboard m_BBoards[DUST_BILLS_CNT];
    D3DXVECTOR2 m_Dirs[DUST_BILLS_CNT];
    D3DXVECTOR2 m_AddSpeed;
    float m_TTL;

    CMatrixEffectDust(const D3DXVECTOR2 &pos, const D3DXVECTOR2 &adddir, float ttl);
    virtual ~CMatrixEffectDust();

public:
    friend class CMatrixEffect;

    virtual void BeforeDraw(void);
    virtual void Draw(void) const override;
    virtual void Takt(float step);
    virtual void Release(void);

    virtual int Priority(void) { return 0; };
};
