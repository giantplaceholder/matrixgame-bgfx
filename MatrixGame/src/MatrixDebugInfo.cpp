// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "MatrixDebugInfo.hpp"
#include "3g.hpp"
#include "MatrixGameDll.hpp"
#include "CBitmap.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <vector>

#include <stupid_logger.hpp>

#define DI_KEY_X 10
#define DI_KEY_Y 10
#define DI_KEY_W 240
#define DI_KEY_H 19
#define DI_VAL_W 400
#define DI_BG_HORIZONTAL_OFFSET 40
#define DI_AVERAGE_CHAR_WIDTH 9     // Can be found with m_Font->GetTextMetricsA(&tm);
#define DI_BG_COLOR 0x40000000

#define MAX_DEBUG_INFO_ITEMS 128

extern IDirect3DDevice9* g_D3DD;

CMatrixDebugInfo::CMatrixDebugInfo()
: m_Font{nullptr}
, m_Pos{DI_KEY_X, DI_KEY_Y}
{
    if (D3D_OK != D3DXCreateFontW(g_D3DD, 25, 0, FW_NORMAL, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                  PROOF_QUALITY, DEFAULT_PITCH, L"MS Sans Serif", &m_Font))
    {
        throw std::runtime_error("Failed to create font with D3DXCreateFontW()");
    }
}

CMatrixDebugInfo::~CMatrixDebugInfo()
{
    m_Font->Release();
};

void CMatrixDebugInfo::Draw()
{
    try
    {
        struct Row { std::wstring text; };
        std::vector<Row> rows;
        for (auto& item : m_Items)
        {
            if (item.bttl > 0)
                continue;
            std::wstring text = item.key;
            if (!item.val.empty())
            {
                if (!text.empty())
                    text += L"  ";
                text += item.val;
            }
            if (!text.empty())
                rows.push_back(Row{std::move(text)});
        }
        if (rows.empty())
            return;

        // DrawText is a stub, rasterize the toast with the host font
        const int limit = g_ScreenX > 80 ? g_ScreenX - 24 : 1000;
        const int slot_h = 36;
        const bool host = g_RangersInterface && g_RangersInterface->m_RangersText;
        CBitmap canvas;
        int text_w = 0;
        int text_h = 0;
        if (host)
        {
            canvas.CreateRGBA(limit, slot_h * int(rows.size()));
            if (canvas.Data())
                std::memset(canvas.Data(), 0, size_t(canvas.Pitch()) * size_t(canvas.SizeY()));
            for (const Row& row : rows)
            {
                CRect clip(0, 0, limit, slot_h);
                SMGDRangersInterfaceText it{};
                g_RangersInterface->m_RangersText(const_cast<wchar*>(row.text.c_str()), const_cast<wchar*>(L"Font.2Normal"),
                                                  0xFFFFFFFFu, limit, slot_h, 0, 0, 1, 0, 0, &clip, &it);
                if (it.m_Buf && it.m_SizeX > 1 && it.m_SizeY > 1 && canvas.Data())
                {
                    CBitmap wrapped;
                    wrapped.CreateRGBA(it.m_SizeX, it.m_SizeY, it.m_Pitch, it.m_Buf);
                    CBitmap owned;
                    owned.CreateRGBA(it.m_SizeX, it.m_SizeY);
                    owned.Copy(CPoint(0, 0), CPoint(it.m_SizeX, it.m_SizeY), wrapped, CPoint(0, 0));
                    owned.SwapByte(CPoint(0, 0), owned.Size(), 0, 2);
                    owned.KeyOpaqueWhite();
                    const int lw = std::min(owned.SizeX(), limit);
                    const int lh = std::min(owned.SizeY(), slot_h);
                    if (owned.Data() && lw > 0 && text_h + lh <= canvas.SizeY())
                    {
                        for (int yy = 0; yy < lh; ++yy)
                        {
                            auto* dst = canvas.Data() + size_t(text_h + yy) * size_t(canvas.Pitch());
                            auto* src = owned.Data() + size_t(yy) * size_t(owned.Pitch());
                            std::memcpy(dst, src, size_t(lw) * 4);
                        }
                        if (lw > text_w)
                            text_w = lw;
                        text_h += lh;
                    }
                }
                if (g_RangersInterface->m_RangersTextClear)
                    g_RangersInterface->m_RangersTextClear(&it);
            }
        }

        const int box_w = std::max(text_w + 16, DI_KEY_W);
        const int box_h = std::max(text_h + 12, DI_KEY_H);
        const float x0 = float(m_Pos.x) - 8.f;
        const float y0 = float(m_Pos.y) - 6.f;
        D3DXVECTOR4 quad[4] = {
            D3DXVECTOR4(x0, y0, 0.5f, 1.0f),
            D3DXVECTOR4(x0 + float(box_w), y0, 0.5f, 1.0f),
            D3DXVECTOR4(x0, y0 + float(box_h), 0.5f, 1.0f),
            D3DXVECTOR4(x0 + float(box_w), y0 + float(box_h), 0.5f, 1.0f),
        };
        g_D3DD->SetRenderState(D3DRS_TEXTUREFACTOR, DI_BG_COLOR);
        SetAlphaOpSelect(0, D3DTA_TFACTOR);
        SetColorOpSelect(0, D3DTA_TFACTOR);
        g_D3DD->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        g_D3DD->SetFVF(D3DFVF_XYZRHW);
        g_D3DD->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(D3DXVECTOR4));

        if (text_w > 0 && text_h > 0 && canvas.Data())
            mgfx_draw_screen_rgba(m_Pos.x, m_Pos.y, text_w, text_h, canvas.Data(), canvas.Pitch());
    }
    catch (...)
    {
    }
}

void CMatrixDebugInfo::T(const wchar_t *key, const wchar_t *val, int ttl, int bttl, bool add)
{
    if (!add)
    {
        auto iter = std::find_if(m_Items.begin(), m_Items.end(), [key](const auto& item) { return item.key == key; });
        if (iter != m_Items.end())
        {
            iter->val = val;
            iter->ttl = ttl;
            iter->bttl = bttl;
            return;
        }
    }

    if (m_Items.size() >= MAX_DEBUG_INFO_ITEMS)
        return;

    m_Items.push_back(SDIItem{key, val, ttl, bttl});
}

void CMatrixDebugInfo::Takt(int ms)
{
    for (auto iter = m_Items.begin(); iter < m_Items.end();)
    {
        if (iter->bttl > 0)
        {
            iter->bttl -= ms;
            ++iter;
            continue;
        }

        iter->ttl -= ms;
        if (iter->ttl <= 0)
        {
            iter = m_Items.erase(iter);
            continue;
        }
        ++iter;
    }
}

void CMatrixDebugInfo::SetStartPos(const CPoint& pos)
{
    m_Pos = pos;
}

void CMatrixDebugInfo::OnLostDevice()
{
    m_Font->OnLostDevice();
}

void CMatrixDebugInfo::OnResetDevice()
{
    m_Font->OnResetDevice();
}