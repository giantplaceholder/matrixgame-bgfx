// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include <new>

#include "MatrixTerSurface.hpp"
#include "MatrixMap.hpp"
#include "MatrixRenderPipeline.hpp"

#include "CFile.hpp"
#include "Mem.hpp"

#include <input.hpp>

CBigIB *CTerSurface::m_BigIB;
CBigVB<CTerSurface::STerSurfVertex> *CTerSurface::m_BigVB;
CBigVB<CTerSurface::STerSurfVertexM> *CTerSurface::m_BigVBM;
CTerSurface *CTerSurface::m_Surfaces;
int CTerSurface::m_SurfacesCnt;

CTerSurface **CTerSurface::m_SurfacesDraw;
int CTerSurface::m_SurfaceLeft;
int CTerSurface::m_SurfaceRite;

CTerSurface::CTerSurface(void) : m_Flags(0), m_Tex(NULL), m_TexGloss(NULL) {
    m_DrawFrameMarker = -1;

    // #ifdef _DEBUG
}

CTerSurface::~CTerSurface() {
    if (m_BigVB) {
        if (m_BigVB->DelSource(&m_VertsSource)) {
            CBigVB<STerSurfVertex>::DestroyBigVB(m_BigVB);
            m_BigVB = NULL;
        }
        HFree(m_VertsSource.verts, g_MatrixHeap);
    }
    if (m_BigVBM) {
        if (m_BigVBM->DelSource(&m_VertsSourceM)) {
            CBigVB<STerSurfVertexM>::DestroyBigVB(m_BigVBM);
            m_BigVBM = NULL;
        }
        HFree(m_VertsSourceM.verts, g_MatrixHeap);
    }
    if (m_BigIB) {
        if (m_BigIB->DelSource(&m_IdxsSource)) {
            CBigIB::DestroyBigIB(m_BigIB);
            m_BigIB = NULL;
        }
        HFree(m_IdxsSource.inds, g_MatrixHeap);
    }
}

void CTerSurface::AllocSurfaces(int n) {
    m_SurfacesCnt = n;
    if (n) {
        m_Surfaces = (CTerSurface *)HAlloc(sizeof(CTerSurface) * n, g_MatrixHeap);
        m_SurfacesDraw = (CTerSurface **)HAllocClear(sizeof(CTerSurface *) * n, g_MatrixHeap);
    }
    else {
        m_Surfaces = NULL;
        m_SurfacesDraw = NULL;
        return;
    }

    for (int i = 0; i < n; i++) {
        new(&m_Surfaces[i]) CTerSurface();
    }
}

void CTerSurface::ClearSurfaces(void) {
    for (int i = 0; i < m_SurfacesCnt; i++) {
        m_Surfaces[i].~CTerSurface();
    }
    if (m_Surfaces)
        HFree(m_Surfaces, g_MatrixHeap);
    if (m_SurfacesDraw)
        HFree(m_SurfacesDraw, g_MatrixHeap);
}

// inline bool InSurf(const D3DXVECTOR2 &p, const D3DXVECTOR2 *points)

void CTerSurface::Load(BYTE *raw) {
    if (m_BigVB == NULL)
        m_BigVB = CBigVB<STerSurfVertex>::NewBigVB(g_MatrixHeap);

    int ids = *(int *)raw;
    raw += sizeof(int);

    m_Index = *(int *)raw;
    raw += sizeof(int);
    m_Color = *(DWORD *)raw;
    raw += sizeof(DWORD);

    std::wstring name;
    if (ids >= 0 && ids < g_MatrixMap->IdsGetCount()) {
        name = g_MatrixMap->IdsGet(ids).GetStrPar(0, L"?");
        if (!name.empty()) m_Tex = (CTextureManaged *)g_Cache->Get(CacheClass::TextureManaged, name.c_str());
    }

    int par = g_MatrixMap->IdsGet(ids).GetCountPar(L"?");
    if (par > 1) {
        for (int i = 1; i < par; ++i) {
            auto parn = g_MatrixMap->IdsGet(ids).GetStrPar(i, L"?");
            auto park = parn.GetStrPar(0, L"=");
            auto parv = parn.GetStrPar(1, L"=");

            if (g_Config.m_LandTexturesGloss && (PAR_TOP_TEX_GLOSS == park)) {
                std::wstring gloss_name;
                CacheReplaceFileNameAndExt(gloss_name, name.c_str(), parv.c_str());
                if (!parv.empty() && CFile::FileExist(name, gloss_name.c_str(), L"png~dds")) {
                    m_TexGloss = (CTextureManaged *)g_Cache->Get(CacheClass::TextureManaged, gloss_name.c_str());
                    SETFLAG(m_Flags, SURFF_GLOSS);
                }
            }
        }
    }

    if (m_Color == 0xFFFFFFFF)
        SETFLAG(m_Flags, SURFF_WHITE);

    DWORD vcnt = *(DWORD *)raw;
    raw += sizeof(DWORD);
    DWORD idxsz = *(DWORD *)raw;
    raw += sizeof(DWORD);
    DWORD grpsc = *(DWORD *)raw;
    raw += sizeof(DWORD);

    m_DispX = *(float *)raw;
    raw += sizeof(float);
    m_DispY = *(float *)raw;
    raw += sizeof(float);

    m_VertsSource.size = sizeof(STerSurfVertex) * vcnt;
    m_VertsSource.verts = (STerSurfVertex *)HAlloc(m_VertsSource.size, g_MatrixHeap);
    for (DWORD i = 0; i < vcnt; ++i) {
        m_VertsSource.verts[i].p.x = *(float *)raw;
        raw += sizeof(float);
        m_VertsSource.verts[i].p.y = *(float *)raw;
        raw += sizeof(float);
        m_VertsSource.verts[i].p.z = *(float *)raw;
        raw += sizeof(float);

        g_MatrixMap->GetNormal(&m_VertsSource.verts[i].n, m_VertsSource.verts[i].p.x, m_VertsSource.verts[i].p.y);

        m_VertsSource.verts[i].color = *(DWORD *)raw;
        raw += sizeof(DWORD);
        m_VertsSource.verts[i].tu = *(float *)raw;
        raw += sizeof(float);
        m_VertsSource.verts[i].tv = *(float *)raw;
        raw += sizeof(float);

        if (!FLAG(m_Flags, SURFF_WRAPY) && (m_VertsSource.verts[i].tv < 0 || m_VertsSource.verts[i].tv > 1))
            SETFLAG(m_Flags, SURFF_WRAPY);
    }

    m_BigVB->AddSource(&m_VertsSource);

    m_IdxsSource.size = idxsz;
    m_IdxsSource.inds = (WORD *)HAlloc(idxsz, g_MatrixHeap);
    memcpy(m_IdxsSource.inds, raw, idxsz);
    raw += idxsz;

    if (m_BigIB == NULL)
        m_BigIB = CBigIB::NewBigIB(g_MatrixHeap);
    m_BigIB->AddSource(&m_IdxsSource);

    for (DWORD i = 0; i < grpsc; ++i) {
        DWORD idx = *(DWORD *)raw;
        raw += sizeof(DWORD);
        CMatrixMapGroup *g = g_MatrixMap->GetGroupByIndex(idx);
        ASSERT(g != NULL);
        g->AddSurface(this);
    }
}
void CTerSurface::LoadM(BYTE *raw) {
    if (m_BigVBM == NULL)
        m_BigVBM = CBigVB<STerSurfVertexM>::NewBigVB(g_MatrixHeap);

    SETFLAG(m_Flags, SURFF_MACRO);

    int ids = *(int *)raw;
    raw += sizeof(int);

    m_Index = *(int *)raw;
    raw += sizeof(int);
    m_Color = *(DWORD *)raw;
    raw += sizeof(DWORD);

    std::wstring name;
    name = g_MatrixMap->IdsGet(ids).GetStrPar(0, L"?");

    m_Tex = (CTextureManaged *)g_Cache->Get(CacheClass::TextureManaged, name.c_str());

    int par = g_MatrixMap->IdsGet(ids).GetCountPar(L"?");
    if (par > 1) {
        for (int i = 1; i < par; ++i) {
            auto parn = g_MatrixMap->IdsGet(ids).GetStrPar(i, L"?");
            auto park = parn.GetStrPar(0, L"=");
            auto parv = parn.GetStrPar(1, L"=");

            if (g_Config.m_LandTexturesGloss && (PAR_TOP_TEX_GLOSS == park)) {
                std::wstring gloss_name;
                CacheReplaceFileNameAndExt(gloss_name, name.c_str(), parv.c_str());
                if (CFile::FileExist(name, gloss_name.c_str(), L"png~dds")) {
                    m_TexGloss = (CTextureManaged *)g_Cache->Get(CacheClass::TextureManaged, gloss_name.c_str());
                    SETFLAG(m_Flags, SURFF_GLOSS);
                }
            }
        }
    }

    if (m_Color == 0xFFFFFFFF)
        SETFLAG(m_Flags, SURFF_WHITE);

    DWORD vcnt = *(DWORD *)raw;
    raw += sizeof(DWORD);
    DWORD idxsz = *(DWORD *)raw;
    raw += sizeof(DWORD);
    DWORD grpsc = *(DWORD *)raw;
    raw += sizeof(DWORD);

    m_DispX = *(float *)raw;
    raw += sizeof(float);
    m_DispY = *(float *)raw;
    raw += sizeof(float);

    m_VertsSourceM.size = sizeof(STerSurfVertexM) * vcnt;
    m_VertsSourceM.verts = (STerSurfVertexM *)HAlloc(m_VertsSourceM.size, g_MatrixHeap);
    for (DWORD i = 0; i < vcnt; ++i) {
        m_VertsSourceM.verts[i].p.x = *(float *)raw;
        raw += sizeof(float);
        m_VertsSourceM.verts[i].p.y = *(float *)raw;
        raw += sizeof(float);
        m_VertsSourceM.verts[i].p.z = *(float *)raw;
        raw += sizeof(float);
        g_MatrixMap->GetNormal(&m_VertsSourceM.verts[i].n, m_VertsSourceM.verts[i].p.x, m_VertsSourceM.verts[i].p.y);
        m_VertsSourceM.verts[i].color = *(DWORD *)raw;
        raw += sizeof(DWORD);
        m_VertsSourceM.verts[i].tu = *(float *)raw;
        raw += sizeof(float);
        m_VertsSourceM.verts[i].tv = *(float *)raw;
        raw += sizeof(float);
        m_VertsSourceM.verts[i].tum = *(float *)raw;
        raw += sizeof(float);
        m_VertsSourceM.verts[i].tvm = *(float *)raw;
        raw += sizeof(float);

        if (!FLAG(m_Flags, SURFF_WRAPY) && (m_VertsSourceM.verts[i].tv < 0 || m_VertsSourceM.verts[i].tv > 1))
            SETFLAG(m_Flags, SURFF_WRAPY);
    }

    m_BigVBM->AddSource(&m_VertsSourceM);

    m_IdxsSource.size = idxsz;
    m_IdxsSource.inds = (WORD *)HAlloc(idxsz, g_MatrixHeap);
    memcpy(m_IdxsSource.inds, raw, idxsz);
    raw += idxsz;

    if (m_BigIB == NULL)
        m_BigIB = CBigIB::NewBigIB(g_MatrixHeap);
    m_BigIB->AddSource(&m_IdxsSource);

    for (DWORD i = 0; i < grpsc; ++i) {
        DWORD idx = *(DWORD *)raw;
        raw += sizeof(DWORD);
        CMatrixMapGroup *g = g_MatrixMap->GetGroupByIndex(idx);
        ASSERT(g != NULL);
        g->AddSurface(this);
    }
}

void CTerSurface::SetFVF(void) {
    g_D3DD->SetFVF(STerSurfVertex::FVF);
}

void CTerSurface::SetFVFM(void) {
    g_D3DD->SetFVF(STerSurfVertexM::FVF);
}

void CTerSurface::BeforeDraw(void) {
    if (m_DrawFrameMarker == g_MatrixMap->GetCurrentFrame())
        return;
    m_DrawFrameMarker = g_MatrixMap->GetCurrentFrame();

    if (m_BigVB)
        m_VertsSource.Prepare(m_BigVB);
    if (m_BigVBM)
        m_VertsSourceM.Prepare(m_BigVBM);

    if (m_BigIB) m_IdxsSource.Prepare(m_BigIB);

    if (m_Tex) m_Tex->Preload();
    if (m_TexGloss)
        m_TexGloss->Preload();

    bool noleft = (m_SurfaceLeft <= 0);
    bool norite = (m_SurfaceRite >= m_SurfacesCnt);

    if (!m_SurfacesDraw || m_SurfacesCnt <= 0) return;
    if (m_SurfaceLeft < 0 || m_SurfaceRite < 0 || m_SurfaceLeft > m_SurfacesCnt || m_SurfaceRite > m_SurfacesCnt) {
        m_SurfaceLeft = 0;
        m_SurfaceRite = 0;
    }
    if (m_SurfaceLeft == m_SurfaceRite) {
        if (m_SurfaceLeft >= m_SurfacesCnt) return;
        m_SurfacesDraw[m_SurfaceLeft] = this;
        ++m_SurfaceRite;
        return;
    }
    // seek index
    int idx;
    int idx0 = m_SurfaceLeft;
    int idx1 = m_SurfaceRite;
    for (;;) {
        idx = ((idx1 - idx0) >> 1) + idx0;
        if (idx < 0 || idx >= m_SurfacesCnt) break;
        CTerSurface* slot = m_SurfacesDraw[idx];
        bool slot_ok = slot >= m_Surfaces && slot < m_Surfaces + m_SurfacesCnt;
        if (!slot_ok || m_Index < slot->m_Index) {
            // left
            if (idx == idx0)
                break;
            idx1 = idx;
        }
        else {
            // rite
            ++idx;
            if (idx == idx1)
                break;
            idx0 = idx;
        }
    }

    if (noleft && norite) {
        if (idx == m_SurfaceRite)
            return;  // far object
        --m_SurfaceRite;
        norite = false;
        goto insert;
    }

    if (!norite && (idx == m_SurfaceRite)) {
        ++m_SurfaceRite;
        m_SurfacesDraw[idx] = this;
    }
    else if (!noleft && (idx == m_SurfaceLeft)) {
        --m_SurfaceLeft;
        m_SurfacesDraw[idx - 1] = this;
    }
    else {
    insert:
        int lc = (idx - m_SurfaceLeft);
        int rc = (m_SurfaceRite - idx);
        bool expand_left = norite || ((lc <= rc) && !noleft);

        if (expand_left) {
            memcpy(&m_SurfacesDraw[m_SurfaceLeft - 1], &m_SurfacesDraw[m_SurfaceLeft], sizeof(CTerSurface *) * lc);
            --m_SurfaceLeft;
            m_SurfacesDraw[idx - 1] = this;
        }
        else {
            memcopy_back_dword(&m_SurfacesDraw[idx + 1], &m_SurfacesDraw[idx], rc);
            ++m_SurfaceRite;
            m_SurfacesDraw[idx] = this;
        }
    }
}

void CTerSurface::DrawAll(void) {
    // g_D3DD->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

    g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    for (int i = m_SurfaceLeft; i < m_SurfaceRite; ++i) {
#ifdef _DEBUG
        if (g_MatrixMap->m_VKeyDown == VK_0) {}
        else
#endif

            if (m_SurfacesDraw && m_SurfacesDraw[i] >= m_Surfaces && m_SurfacesDraw[i] < m_Surfaces + m_SurfacesCnt)
                m_SurfacesDraw[i]->Draw();
    }

    g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

    g_D3DD->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
}

void CTerSurface::Draw(void) {
    // #ifdef _DEBUG

    int vbase = -1;
    int vcnt;
    if (FLAG(m_Flags, SURFF_MACRO)) {
        if (m_BigVBM) {
            vbase = m_VertsSourceM.Select(m_BigVBM);
            vcnt = m_VertsSourceM.size / sizeof(STerSurfVertexM);
            SetFVFM();
        }
    }
    else {
        if (m_BigVB) {
            vbase = m_VertsSource.Select(m_BigVB);
            vcnt = m_VertsSource.size / sizeof(STerSurfVertex);
            SetFVF();
        }
    }

    if (vbase < 0)
        return;

    int ibase = m_IdxsSource.Select(m_BigIB);
    if (ibase < 0)
        return;

    D3DMATRIX m;
    m = g_MatrixMap->GetIdentityMatrix();
    m._41 = m_DispX;
    m._42 = m_DispY;

    ASSERT_DX(g_D3DD->SetTransform(D3DTS_WORLD, &m));

    g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR, m_Color);

    int type = m_Flags & SURF_FLAG_MASK;

    // static int iii =0;

    for (int pass = 0; pass < g_Render->m_TerSurfPass[type]; ++pass) {
        g_Render->m_TerSurfTex[type](m_Tex, m_TexGloss, pass);
        g_Render->m_TerSurf[type](pass, FLAG(m_Flags, SURFF_WRAPY));

        // if (iii == m_Index)
        {
            // ASSERT_DX(g_D3DD->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,vbase,0,vcnt, ibase, m_IdxsSource.size / (3 *
            // sizeof(WORD))));
            ASSERT_DX(g_D3DD->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, vbase, 0, vcnt, ibase,
                                                   m_IdxsSource.size / sizeof(WORD) - 2));
        }
    }
    g_Render->m_TerSurfClear[type]();
}
