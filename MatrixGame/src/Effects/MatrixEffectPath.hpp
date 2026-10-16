// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

// path
#define PATH_DOT_DISTANCE  6
#define PATH_DOT_SPEED     0.007f
#define PATH_DOT_RATE      500
#define PATH_SIZE          1.5f
#define PATH_COLOR         0xFFFFFF10
#define PATH_HIDE_DISTANCE 30

//___________________________________________________________________________________________________
/////////////////////////////////////////////////////////////////////////////////////////////////////

struct SPathDot {
    SPathDot *next;
    SPathDot *prev;

    CBillboard dot;
    float pos;
};

class CMatrixEffectPath : public CMatrixEffect {
    DWORD m_Kill;

    CBuf m_Points;
    CBuf m_Dirs;
    CBuf m_Lens;

    int m_DotsCnt;
    int m_DotsMax;
    SPathDot *m_Dots;

    int m_Cnt;
    float m_Len;
    float _m_Len;

    // float   m_Time;
    float m_Takt;
    float m_NextTakt;

    SPathDot *m_First;
    SPathDot *m_Last;

    float m_Angle;
    float m_Barier;

    CMatrixEffectPath(const D3DXVECTOR3 *pos, int cnt);
    virtual ~CMatrixEffectPath();

    void UpdateData(void);

public:
    friend class CMatrixEffect;

    void GetPos(D3DXVECTOR3 *out, float t);
    void AddPos(const D3DXVECTOR3 &pos);

    virtual void BeforeDraw(void);
    virtual void Draw(void) const override;
    virtual void Takt(float step);
    virtual void Release(void);

    virtual int Priority(void) { return MAX_EFFECT_PRIORITY; };

    void Kill(void) { m_Kill = true; }
};
