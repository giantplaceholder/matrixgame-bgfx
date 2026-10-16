// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#ifndef SHADOW_STENCIL_INCLUDE
#define SHADOW_STENCIL_INCLUDE

#include "VectorObject.hpp"

#include <vector>

using SVOShadowStencilVertex = D3DXVECTOR3;

class CVOShadowStencil
{
    DWORD m_DirtyDX;
    D3D_VB m_VB;
    D3D_IB m_IB;

    int m_IBSize;  // in bytes
    int m_VBSize;

    static CVOShadowStencil *m_First;
    static CVOShadowStencil *m_Last;

    CVOShadowStencil *m_Prev;
    CVOShadowStencil *m_Next;

    CVectorObject *m_vo;

    int m_FrameFor;

    struct SSSFrameData {
        std::vector<SVOShadowStencilVertex> m_preVerts;
        std::vector<WORD> m_preInds;

        SVONormal m_light;
        float m_len;
    };

    std::vector<SSSFrameData> m_Frames;
    int m_FramesCnt;

public:
    static void StaticInit(void)
    {
        m_First = NULL;
        m_Last = NULL;
    }
    static void BeforeRenderAll(void);

    static void MarkAllBuffersNoNeed(void);

    CVOShadowStencil();
    ~CVOShadowStencil();

    bool IsReady(void) const { return !m_Frames.empty(); }
    void DX_Prepare(void);
    void DX_Free(void) {
        if (IS_VB(m_VB))
            DESTROY_VB(m_VB);
        if (IS_VB(m_IB))
            DESTROY_VB(m_IB);

        m_DirtyDX = true;
    }

    void Build(CVectorObject &obj, int frame, const D3DXVECTOR3 &vLight, float len, bool invert);

    void BeforeRender(void) { DX_Prepare(); }
    void Render(const D3DXMATRIX &objma);
};

#endif