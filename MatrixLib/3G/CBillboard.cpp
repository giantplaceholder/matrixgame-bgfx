// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "3g.hpp"

#include "CBillboard.hpp"

#include "preallocated_list.hpp"

#include <algorithm>
#include <vector>

static constexpr int MAX_BB_TEXGROUPS = 64;
static constexpr int MAX_BBOARDS = 8196;

CTextureManaged* CBillboard::m_SortableTex{nullptr};
// CTextureManaged * CBillboard::m_IntenseTex{nullptr};

D3D_VB CBillboard::m_VB{nullptr};
D3D_IB CBillboard::m_IB{nullptr};

static std::vector<std::pair<double, const CBillboard*>> _bboards;
static preallocated_list<CBillboard> _intense_queue;
static preallocated_list<CBillboardLine> _draw_queue;

/////////////////////////////////////////////////////////////

void CBillboard::SortEndDraw(const D3DXMATRIX &iview, const D3DXVECTOR3 &campos) {
    DTRACE();

    if (m_VB == NULL || m_IB == NULL)
        Init();

    int ns = 0, ni = 0;
    SBillboardVertex *vb;

    if (!_bboards.empty())
    {
        ns = _bboards.size();

        LOCK_VB_DYNAMIC(m_VB, &vb);

        for (auto bb : _bboards)
        {
            bb.second->UpdateVBSlot(vb, iview);
            vb += 4;
        }
    }

    struct SDrawBillGroup {
        int cntv;  // vertices
        int cntt;  // triangles
        // int vbase;    // no need, always 0
        int ibase;
        int min_idx;
        CTextureManaged *tex;
    };

    SDrawBillGroup groups[MAX_BB_TEXGROUPS];
    int groupscnt = 0;

    if (!_intense_queue.empty() || !_draw_queue.empty())
    {
        int ost = (MAX_BBOARDS * 2) - ns;

        if (ns == 0) {
            // lock VB/IB if not locked yet
            LOCK_VB_DYNAMIC(m_VB, &vb);
        }

        int n = 0, minvi = ns * 4;

        groups[0].tex = NULL;

        int ibt_ = ns * 6;

        const auto process_queue = [&](auto& queue, const auto& update_param) {
            for (auto iter = queue.begin(); iter != queue.end(); ++iter)
            {
                const auto* bl = *iter;
                bl->UpdateVBSlot(vb, update_param);
                vb += 4;

                if (groups[groupscnt].tex == NULL) {
                    // new group
                    groups[groupscnt].tex = bl->m_Tex;
                    groups[groupscnt].min_idx = minvi;
    #ifdef _DEBUG
                    if ((uintptr_t)groups[groupscnt].tex == 0xACACACAC)
                        debugbreak();
    #endif
                    // groups[groupscnt].vbase = base;
                    groups[groupscnt].ibase = ibt_;
                }

                ++n;
                ++ni;
                minvi += 4;
                ibt_ += 6;
                --ost;

                auto next = iter.next();
                if (!next || bl->m_Tex != (*next)->m_Tex)
                {
                    // end of group
                    groups[groupscnt].cntv = n * 4;
                    groups[groupscnt].cntt = n * 2;
                    if (++groupscnt >= MAX_BB_TEXGROUPS)
                        break;
                    groups[groupscnt].tex = NULL;

                    n = 0;
                }

                if (ost <= 0)
                    break;
            }
        };

        process_queue(_intense_queue, iview);

        if (groupscnt < MAX_BB_TEXGROUPS && ost > 0)
        {
            ASSERT(n == 0);
            process_queue(_draw_queue, campos);
            ASSERT((ost > 0 && n == 0) || ost <= 0);
        }

        _draw_queue.clear();

        UNLOCK_VB(m_VB);
    }
    else {
        if (ns > 0) {
            UNLOCK_VB(m_VB);
        }
    }

    // draw sorted

    if (ns > 0) {
        ASSERT_DX(g_D3DD->SetFVF(BILLBOARD_FVF));

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF, 0));

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_LIGHTING, FALSE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));

        ASSERT_DX(g_D3DD->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE));
        ASSERT_DX(g_D3DD->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE));
        ASSERT_DX(g_D3DD->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0));
        ASSERT_DX(g_D3DD->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1));

        ASSERT_DX(g_D3DD->SetTextureStageState(0, D3DTSS_RESULTARG, D3DTA_CURRENT));
        ASSERT_DX(g_D3DD->SetTextureStageState(1, D3DTSS_RESULTARG, D3DTA_CURRENT));

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));

        SetColorOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
        SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
        SetColorOpDisable(1);

        ASSERT_DX(g_D3DD->SetStreamSource(0, GET_VB(m_VB), 0, sizeof(SBillboardVertex)));
        ASSERT_DX(g_D3DD->SetIndices(GET_IB(m_IB)));

        ASSERT_DX(g_D3DD->SetTexture(0, m_SortableTex->Tex()));

        ASSERT_DX(g_D3DD->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, ns * 4, 0, ns * 2));
    }

    if (ni > 0) {
        ASSERT_DX(g_D3DD->SetFVF(BILLBOARD_FVF));

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
        // alpha test ate the soft edge
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF, 0));

        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
        ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));

        SetColorOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
        SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
        SetColorOpDisable(1);

        g_D3DD->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        // z was clipping the cage
        g_D3DD->SetRenderState(D3DRS_ZENABLE, FALSE);

        g_D3DD->SetStreamSource(0, GET_VB(m_VB), 0, sizeof(SBillboardVertex));
        ASSERT_DX(g_D3DD->SetIndices(GET_IB(m_IB)));

        for (int i = 0; i < groupscnt; ++i) {
            groups[i].tex->Preload();
            ASSERT_DX(g_D3DD->SetTexture(0, groups[i].tex->Tex()));
            ASSERT_DX(g_D3DD->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, groups[i].min_idx, groups[i].cntv,
                                                   groups[i].ibase, groups[i].cntt));
        }
    }

    g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE));
    g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));
    g_D3DD->SetRenderState(D3DRS_ZENABLE, TRUE);

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHAREF, 0x08));

    // m_First = NULL;
    _bboards.clear();
    // m_Root = NULL;

    _intense_queue.clear();
}

void CBillboard::SortIntense(void) const
{
    auto iter =
        std::find_if(
            _intense_queue.begin(),
            _intense_queue.end(),
            [&](const auto& item) {
                return m_Tex == item->m_Tex;
            }
        );

    _intense_queue.insert(iter, this);
}

void CBillboard::Sort(const D3DXMATRIX &sort) const
{
    if (IsIntense())
    {
        SortIntense();
        return;
    }

    if (_bboards.size() == MAX_BBOARDS)
    {
        return;
    }

    // here was a smart-ass optimized algorithm to build a sorted list of billboard in a plain array
    // trying to minimize memory copying/moving, but still using them to insert the element into
    // correct position.
    // i've thrown in away for the sake of code readability. now we'll have a vector here, which will
    // most probably optimize all the movement under the hood anyway as long as we're storing POD types,
    // but will do it in 3 lines of code instead of 50.
    // yes, pessimization. on modern platform most probably it's not. shall not try to optimize it
    // until performance gain is explicitly proven.

    double z_index = Double2Int((sort._13 * m_Pos.x + sort._23 * m_Pos.y + sort._33 * m_Pos.z + sort._43) * 256.0);

    auto comp = [](const double new_value, const auto item) { return new_value < item.first; };
    auto position = std::upper_bound(_bboards.begin(), _bboards.end(), z_index, comp);
    _bboards.emplace(position, z_index, this);
}

CBillboard::~CBillboard() {
    DTRACE();
    // do nothing!
#ifdef _DEBUG
    if (!_intense_queue.empty())
    // if (m_FirstIntense != NULL || m_Root != NULL)
    // if (m_FirstIntense != NULL || m_First != NULL)
    {
        debugbreak();
    }
#endif
#ifdef _DEBUG
    if (!release_called) {
        debugbreak();
    }
#endif
}

void CBillboard::Release(void) {
    DTRACE();

#ifdef _DEBUG
    if (!_intense_queue.empty())
    {
        debugbreak();
    }
    release_called = true;
#endif
}

void CBillboard::Init(void) {
    DTRACE();

    if (m_VB == NULL) {
        CREATE_VB_DYNAMIC(MAX_BBOARDS * 2 * 4 * sizeof(SBillboardVertex), BILLBOARD_FVF, m_VB);
    }

    if (m_IB == NULL) {
        CREATE_IB16(MAX_BBOARDS * 6, m_IB);

        if (m_IB != NULL) {
            WORD *p;
            LOCK_IB(m_IB, &p);
            for (int i = 0; i < MAX_BBOARDS; ++i) {
                DWORD vol = i * 4;
                *(DWORD *)p = vol | ((vol + 1) << 16);
                *(DWORD *)(p + 2) = (vol + 2) | ((vol + 1) << 16);
                *(DWORD *)(p + 4) = (vol + 3) | ((vol + 2) << 16);

                p += 6;
            }
            UNLOCK_IB(m_IB);
        }
    }
}

void CBillboard::Deinit(void) {
    if (m_VB != NULL)
        DESTROY_VB(m_VB);
    if (m_IB != NULL)
        DESTROY_IB(m_IB);
}

CBillboard::CBillboard(TRACE_PARAM_DEF const D3DXVECTOR3 &pos, float scale, float angle, DWORD color,
                       const SBillboardTexture *tex)
  :
#ifdef _DEBUG
    m_file(_file), m_line(_line),
#endif
    m_Pos(pos), m_Scale(scale), m_Color(color), m_Texture(tex) {
    DTRACE();

    SetIntense(false);
    SetAngle(angle, 0, 0);

#ifdef _DEBUG
    release_called = false;
#endif
}

CBillboard::CBillboard(TRACE_PARAM_DEF const D3DXVECTOR3 &pos, float scale, float angle, DWORD color,
                       CTextureManaged *tex)
  :  // intense
#ifdef _DEBUG
    m_file(_file), m_line(_line),
#endif
    m_Pos(pos), m_Scale(scale), m_Color(color), m_Tex(tex) {
    DTRACE();

    SetIntense(true);
    SetAngle(angle, 0, 0);

#ifdef _DEBUG
    release_called = false;
#endif
}

void CBillboard::UpdateVBSlot(SBillboardVertex *vb, const D3DXMATRIX &iview) const
{
    static const D3DXVECTOR3 base[4] = {
            D3DXVECTOR3(-1, -1, 0),
            D3DXVECTOR3(-1, 1, 0),
            D3DXVECTOR3(1, -1, 0),
            D3DXVECTOR3(1, 1, 0),
    };

    float r11 = m_Rot._11 * m_Scale;
    float r12 = m_Rot._12 * m_Scale;
    float r21 = m_Rot._21 * m_Scale;
    float r22 = m_Rot._22 * m_Scale;

    D3DXMATRIX m(

            (r11 * iview._11 + r12 * iview._21), (r11 * iview._12 + r12 * iview._22),
            (r11 * iview._13 + r12 * iview._23), 0,

            (r21 * iview._11 + r22 * iview._21), (r21 * iview._12 + r22 * iview._22),
            (r21 * iview._13 + r22 * iview._23), 0,

            (iview._31) * m_Scale, (iview._32) * m_Scale, (iview._33) * m_Scale, 0,

            m_Rot._41 * iview._11 + m_Rot._42 * iview._21 + m_Pos.x,
            m_Rot._41 * iview._12 + m_Rot._42 * iview._22 + m_Pos.y,
            m_Rot._41 * iview._13 + m_Rot._42 * iview._23 + m_Pos.z, 1);

    D3DXVec3TransformCoordArray(&vb->p, sizeof(SBillboardVertex), base, sizeof(D3DXVECTOR3), &m, 4);

    if (IsIntense()) {
        vb->color = m_Color;
        vb->tu = 0;
        vb->tv = 1;

        ++vb;

        vb->color = m_Color;
        vb->tu = 0;
        vb->tv = 0;

        ++vb;

        vb->color = m_Color;
        vb->tu = 1;
        vb->tv = 1;

        ++vb;

        vb->color = m_Color;
        vb->tu = 1;
        vb->tv = 0;
    }
    else {
        vb->color = m_Color;
        vb->tu = m_Texture->tu0;
        vb->tv = m_Texture->tv1;

        ++vb;

        vb->color = m_Color;
        vb->tu = m_Texture->tu0;
        vb->tv = m_Texture->tv0;

        ++vb;

        vb->color = m_Color;
        vb->tu = m_Texture->tu1;
        vb->tv = m_Texture->tv1;

        ++vb;

        vb->color = m_Color;
        vb->tu = m_Texture->tu1;
        vb->tv = m_Texture->tv0;
    }
}

void CBillboard::DrawNow(const D3DXMATRIX &iview) {
    if (!IsIntense())
        return;

    ASSERT_DX(g_D3DD->SetFVF(BILLBOARD_FVF));

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));

    // leftover src blend made the glow solid
    g_D3DD->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    SetColorOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
    SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
    SetColorOpDisable(1);

    SBillboardVertex vb[4];

    UpdateVBSlot(vb, iview);

    m_Tex->Preload();
    g_D3DD->SetTexture(0, m_Tex->Tex());

    D3DXMATRIX wrld;
    D3DXMatrixIdentity(&wrld);
    g_D3DD->SetTransform(D3DTS_WORLD, &wrld);

    g_D3DD->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &vb, sizeof(SBillboardVertex));

    g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE));
    g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

CBillboardLine::CBillboardLine(TRACE_PARAM_DEF const D3DXVECTOR3 &pos0, const D3DXVECTOR3 &pos1, float width,
                               DWORD color, CTextureManaged *tex)
  :
#ifdef _DEBUG
    m_file(_file), m_line(_line),
#endif
    CMain(), m_Pos0(pos0), m_Pos1(pos1), m_Width(width), m_Color(color), m_Tex(tex) {

#ifdef _DEBUG
    release_called = false;
#endif
}

CBillboardLine::~CBillboardLine()
{
    // do nothing
#ifdef _DEBUG
    if (!_draw_queue.empty())
    {
        debugbreak();
    }
    if (!release_called)
    {
        debugbreak();
    }
#endif
}

void CBillboardLine::Release(void)
{
    DTRACE();

#ifdef _DEBUG
    release_called = true;
#endif
    ASSERT(_draw_queue.empty());
}

void CBillboardLine::UpdateVBSlot(SBillboardVertex *vb, const D3DXVECTOR3 &campos) const {
    static const D3DXVECTOR3 base[4] = {
            D3DXVECTOR3(0, 0.5f, 0),
            D3DXVECTOR3(0, -0.5f, 0),
            D3DXVECTOR3(1, 0.5f, 0),
            D3DXVECTOR3(1, -0.5f, 0),
    };

    D3DXMATRIX m_Mat;

    D3DXVECTOR3 dir(m_Pos1 - m_Pos0),
            viewdir(campos.x - (m_Pos0.x + m_Pos1.x) * 0.5f, campos.y - (m_Pos0.y + m_Pos1.y) * 0.5f,
                    campos.z - (m_Pos0.z + m_Pos1.z) * 0.5f);

    m_Mat._11 = dir.x;  // x1
    m_Mat._12 = dir.y;  // y1
    m_Mat._13 = dir.z;  // z1
    m_Mat._14 = 0;

    m_Mat._31 = viewdir.x;  // x2
    m_Mat._32 = viewdir.y;  // y2
    m_Mat._33 = viewdir.z;  // z2
    m_Mat._34 = 0;

    D3DXVECTOR3 left((m_Mat._12 * m_Mat._33 - m_Mat._13 * m_Mat._32), (m_Mat._13 * m_Mat._31 - m_Mat._11 * m_Mat._33),
                     (m_Mat._11 * m_Mat._32 - m_Mat._12 * m_Mat._31));

    D3DXVec3Normalize(&left, &left);

    m_Mat._21 = left.x * m_Width;
    m_Mat._22 = left.y * m_Width;
    m_Mat._23 = left.z * m_Width;
    m_Mat._24 = 0;

    m_Mat._41 = m_Pos0.x;
    m_Mat._42 = m_Pos0.y;
    m_Mat._43 = m_Pos0.z;
    m_Mat._44 = 1;

    D3DXVec3TransformCoordArray(&vb->p, sizeof(SBillboardVertex), base, sizeof(D3DXVECTOR3), &m_Mat, 4);

    vb->color = m_Color;
    vb->tu = 0;
    vb->tv = 1;

    ++vb;

    vb->color = m_Color;
    vb->tu = 0;
    vb->tv = 0;

    ++vb;

    vb->color = m_Color;
    vb->tu = 1;
    vb->tv = 1;

    ++vb;

    vb->color = m_Color;
    vb->tu = 1;
    vb->tv = 0;
}

void CBillboardLine::DrawNow(const D3DXVECTOR3 &campos) const
{
    ASSERT_DX(g_D3DD->SetFVF(BILLBOARD_FVF));

    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));

    g_D3DD->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    SetColorOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
    SetAlphaOpAnyOrder(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
    SetColorOpDisable(1);

    SBillboardVertex vb[4];

    UpdateVBSlot(vb, campos);

    m_Tex->Preload();
    g_D3DD->SetTexture(0, m_Tex->Tex());

    D3DXMATRIX wrld;
    D3DXMatrixIdentity(&wrld);
    g_D3DD->SetTransform(D3DTS_WORLD, &wrld);

    g_D3DD->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &vb, sizeof(SBillboardVertex));

    g_D3DD->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE));
    g_D3DD->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    ASSERT_DX(g_D3DD->SetRenderState(D3DRS_ZWRITEENABLE, TRUE));
}

void CBillboardLine::AddToDrawQueue(void) const
{
    auto iter =
        std::find_if(
            _draw_queue.begin(),
            _draw_queue.end(),
            [&](const auto& item) {
                return m_Tex == item->m_Tex;
            }
        );

    _draw_queue.insert(iter, this);
}