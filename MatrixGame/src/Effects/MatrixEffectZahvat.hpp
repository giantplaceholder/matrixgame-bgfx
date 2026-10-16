// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include "MatrixEffect.hpp"
#include "MatrixEffectBillboard.hpp"

class CMatrixEffectZahvat : public CMatrixEffect
{
    int m_Count;
    CMatrixEffectBillboard *m_BBoards;

    CMatrixEffectZahvat(const D3DXVECTOR3 &pos, float radius, float angle, int cnt);
    virtual ~CMatrixEffectZahvat();

public:
    friend class CMatrixEffect;

    void UpdateData(DWORD color, int count);

    virtual void BeforeDraw(void);
    virtual void Draw(void) const override;
    virtual void Takt(float step);
    virtual void Release(void);

    virtual int Priority(void) { return MAX_EFFECT_PRIORITY; };
};
