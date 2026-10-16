// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "Texture.hpp"

#include "CFile.hpp"
#include "CStorage.hpp"

#include <cstdio>
#include <cstdint>

#include <list>

static std::list<CBaseTexture*> _textures;

void CBaseTexture::OnLostDevice(void) {
    for (auto tex : _textures)
    {
        if (!FLAG(tex->m_Flags, TF_LOST) && tex->m_Type == CacheClass::Texture)
            ((CTexture *)tex)->OnLostDevice();
    }
}
void CBaseTexture::OnResetDevice(void) {
    for (auto tex : _textures)
    {
        if (FLAG(tex->m_Flags, TF_LOST) && tex->m_Type == CacheClass::Texture)
            ((CTexture *)tex)->OnResetDevice();
    }
}

CBaseTexture::CBaseTexture(void)
: CCacheData()
{
    _textures.push_back(this);
}

CBaseTexture::~CBaseTexture()
{
    _textures.remove(this);
};

LPDIRECT3DTEXTURE9 CBaseTexture::LoadTextureFromFile(bool to16, D3DPOOL pool) {
    DTRACE();
    LPDIRECT3DTEXTURE9 ret = NULL;
    ParseFlags(m_Name);
    if (to16) {
        if (FLAG(m_Flags, TF_COMPRESSED)) {
            goto autoload;
        }
        std::wstring tn(m_Name);
        CFile::FileExist(tn, tn.c_str(), CacheExtsTex);
        auto idx = tn.rfind('.');
        if (idx == std::wstring::npos)
            goto autoload;
        utils::to_lower(tn, idx + 1);
        if (0 != memcmp(tn.c_str() + idx + 1, L"png", sizeof(wchar) * 4))
            goto autoload;

        CTextureManaged *tex = CACHE_CREATE_TEXTUREMANAGED();
        CBitmap bm;
        bm.LoadFromPNG(tn.c_str());
        tex->LoadFromBitmap(bm, true, FLAG(m_Flags, TF_NOMIPMAP) ? 1 : 0);

#ifdef USE_DX_MANAGED_TEXTURES
        ret = tex->m_Tex;
        tex->m_Tex = NULL;
#else
        ret = tex->m_TexFrom;
        tex->m_TexFrom = NULL;
#endif

        CCache::Destroy(tex);

        // ASSERT(pool == D3DPOOL_SYSTEMMEM);

        return ret;
    }

autoload:

    CBuf buf;
    if (FLAG(m_Flags, TF_COMPRESSED)) {
        LoadFromFile(buf, CacheExtsTex);

        CStorage ccc(g_CacheHeap);
        ccc.Load(buf);

        CDataBuf *b = ccc.GetBuf(L"0", L"0", ST_BYTE);
        buf.Clear();
        buf.Add(b->GetFirst<BYTE>(0), b->GetArrayLength(0));
    }
    else {
        LoadFromFile(buf, CacheExtsTex);
    }

    if (FAILED(D3DXCreateTextureFromFileInMemoryEx(g_D3DD, buf.Get(), buf.Len(), 0, 0,
                                                   FLAG(m_Flags, TF_NOMIPMAP) ? 1 : 0, 0, D3DFMT_UNKNOWN, pool,
                                                   D3DX_FILTER_NONE, D3DX_FILTER_BOX, 0, NULL, NULL, &ret))) {
        return NULL;
    }
    if (ret) {
#if defined(MATRIXGAME_BGFX)
        static int logged = 0;
        if (logged < 6) {
            ++logged;
            const uint8_t* b = static_cast<const uint8_t*>(buf.Get());
            std::printf("texfile bytes %u magic %d,%d,%d,%d out %ux%u\n",
                        unsigned(buf.Len()),
                        buf.Len() >= 4 ? int(b[0]) : -1,
                        buf.Len() >= 4 ? int(b[1]) : -1,
                        buf.Len() >= 4 ? int(b[2]) : -1,
                        buf.Len() >= 4 ? int(b[3]) : -1,
                        ret->width, ret->height);
        }
#endif
    }
    return ret;
}

void CBaseTexture::ParseFlags(const ParamParser& name) {
    m_Flags = 0;
    std::wstring tstr;
    int cnt = name.GetCountPar(L"?");
    if (cnt > 1) {
        for (int i = 1; i < cnt; i++) {
            tstr = utils::trim(name.GetStrPar(i, L"?"));
            if (tstr == L"Trans") {
                SETFLAG(m_Flags, TF_ALPHATEST);
            }
            else if (tstr == L"Alpha") {
                SETFLAG(m_Flags, TF_ALPHABLEND);
            }
        }
    }

    tstr = name.GetStrPar(0, L"?");
    CacheReplaceFileExt(tstr, tstr.c_str(), L".txt");

    if (CFile::FileExist(tstr, tstr.c_str())) {
        bool proceed = true;
        if (tstr.find(L"pinguin.txt") != std::wstring::npos || tstr.find(L"robotarget.txt") != std::wstring::npos)
        {
            proceed = false;
        }

        if (proceed) {
            CBlockPar texi;
            texi.LoadFromTextFile(tstr);

            tstr = texi.ParGetNE(L"AlphaTest");
            if (!tstr.empty()) {
                if (tstr == L"0") {
                    RESETFLAG(m_Flags, TF_ALPHATEST);
                }
                else {
                    SETFLAG(m_Flags, TF_ALPHATEST);
                }
            }
            tstr = texi.ParGetNE(L"AlphaBlend");
            if (!tstr.empty()) {
                if (tstr == L"0") {
                    RESETFLAG(m_Flags, TF_ALPHABLEND);
                }
                else {
                    SETFLAG(m_Flags, TF_ALPHABLEND);
                }
            }
            tstr = texi.ParGetNE(L"Compressed");
            if (!tstr.empty()) {
                if (tstr == L"0") {
                    RESETFLAG(m_Flags, TF_COMPRESSED);
                }
                else {
                    SETFLAG(m_Flags, TF_COMPRESSED);
                }
            }
        }
    }
}

// //////////////////////////////////////////////////////////////////////////////

void CTexture::OnLostDevice(void) {
    DTRACE();

    // store texture
    if (m_Tex == NULL)
        return;

    D3DSURFACE_DESC desc;
    HRESULT res = m_Tex->GetLevelDesc(0, &desc);
    if (res != D3D_OK) {
        // oblom konkretny
        SETFLAG(m_Flags, TF_LOST);
        m_Tex->Release();
        m_Tex = NULL;
        return;
    }

    if (desc.Pool != D3DPOOL_DEFAULT)
        return;  // do not need any actions

    SETFLAG(m_Flags, TF_LOST);

    IDirect3DTexture9 *tex;

    int levels = m_Tex->GetLevelCount();

    res = g_D3DD->CreateTexture(desc.Width, desc.Height, levels, 0, desc.Format, D3DPOOL_SYSTEMMEM, &tex, 0);
    if (res != D3D_OK) {
        // oblom konkretny
        m_Tex->Release();
        m_Tex = NULL;
        return;
    }

    m_Usage = desc.Usage;

    for (int i = 0; i < levels; ++i) {
        IDirect3DSurface9 *from;
        IDirect3DSurface9 *to;

        if (D3D_OK != tex->GetSurfaceLevel(i, &to)) {
            m_Tex->Release();
            m_Tex = NULL;
            tex->Release();
            return;
        }

        if (D3D_OK != m_Tex->GetSurfaceLevel(i, &from)) {
            to->Release();
            m_Tex->Release();
            m_Tex = NULL;
            tex->Release();
            return;
        }

        if (D3D_OK != D3DXLoadSurfaceFromSurface(to, NULL, NULL, from, NULL, NULL, D3DX_FILTER_NONE, 0)) {
            from->Release();
            to->Release();
            m_Tex->Release();
            m_Tex = NULL;
            tex->Release();
            return;
        }

        from->Release();
        to->Release();
    }
    m_Tex->Release();
    m_Tex = tex;
}

void CTexture::OnResetDevice(void) {
    DTRACE();

    if (!FLAG(m_Flags, TF_LOST) || m_Tex == NULL)
        return;  // not lost

    // restore texture

    D3DSURFACE_DESC desc;
    HRESULT res = m_Tex->GetLevelDesc(0, &desc);
    if (res != D3D_OK) {
        // oblom konkretny
        SETFLAG(m_Flags, TF_LOST);
        m_Tex->Release();
        m_Tex = NULL;
        return;
    }

    IDirect3DTexture9 *tex;

    int levels = m_Tex->GetLevelCount();

    res = g_D3DD->CreateTexture(desc.Width, desc.Height, levels, m_Usage, desc.Format, D3DPOOL_DEFAULT, &tex, 0);
    if (res != D3D_OK) {
        // oblom konkretny
        m_OOM_counter = 50;
        m_Tex->Release();
        m_Tex = NULL;
        return;
    }

    for (int i = 0; i < levels; ++i) {
        IDirect3DSurface9 *from;
        IDirect3DSurface9 *to;

        if (D3D_OK != tex->GetSurfaceLevel(i, &to)) {
            m_OOM_counter = 50;
            m_Tex->Release();
            m_Tex = NULL;
            tex->Release();
            return;
        }

        if (D3D_OK != m_Tex->GetSurfaceLevel(i, &from)) {
            m_OOM_counter = 50;
            to->Release();
            m_Tex->Release();
            m_Tex = NULL;
            tex->Release();
            return;
        }

        m_Tex->AddDirtyRect(NULL);
        if (D3D_OK != g_D3DD->UpdateTexture(m_Tex, tex)) {
            m_OOM_counter = 50;
            from->Release();
            to->Release();
            m_Tex->Release();
            m_Tex = NULL;
            tex->Release();
            return;
        }

        from->Release();
        to->Release();
    }
    m_Tex->Release();
    m_Tex = tex;
    m_OOM_counter = 0;

    RESETFLAG(m_Flags, TF_LOST);
}

void CTexture::LoadFromBitmap(CBitmap &bm) {
    DTRACE();

#ifdef _DEBUG
    ASSERT(!FLAG(g_Flags, GFLAG_RENDERINPROGRESS));
#endif

    Unload();
    CBuf buf;
    bm.SaveInDDSUncompressed(buf);
    FAILED_DX(D3DXCreateTextureFromFileInMemoryEx(
            g_D3DD, buf.Get(), buf.Len(), 0, 0, FLAG(m_Flags, TF_NOMIPMAP) ? 1 : 0, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER, D3DX_FILTER_BOX, 0, NULL, NULL, &m_Tex));
}

void CTexture::Unload(void) {
    DTRACE();

    if (m_Tex) {
        m_Tex->Release();
        m_Tex = NULL;
    }
    m_Flags = 0;
}

void CTexture::Load(void) {
    DTRACE();

    if (FLAG(m_Flags, TF_LOST))
        return;  // нехуй тут делать. текстура потеряна блять!

    ASSERT(!IsLoaded());
    // Unload();

    if (g_D3DD->GetAvailableTextureMem() < (2 * 1024 * 1024)) {
        m_OOM_counter = OOM_TEXTRUE_HIT_COUNTER;
        return;
    }

    m_Tex = LoadTextureFromFile(false);
    if (m_Tex == NULL) {
        m_OOM_counter = OOM_TEXTRUE_HIT_COUNTER;
        return;
    }

    D3DSURFACE_DESC desc;
    m_Tex->GetLevelDesc(0, &desc);
    m_Size.x = desc.Width;
    m_Size.y = desc.Height;
}

void CTexture::MakeSolidTexture(int sx, int sy, DWORD color) {
#ifdef _DEBUG
    ASSERT(!FLAG(g_Flags, GFLAG_RENDERINPROGRESS));
#endif

    Unload();
    D3DLOCKED_RECT lr;
    LPDIRECT3DTEXTURE9 sm_tex;
    ASSERT_DX(g_D3DD->CreateTexture(sx, sy, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &sm_tex, 0));
    ASSERT_DX(sm_tex->LockRect(0, &lr, NULL, 0 /*D3DLOCK_DISCARD_TEMP*/));
    int SizeInMemory = lr.Pitch * sy;
    DWORD *des = (DWORD *)lr.pBits;
    while (SizeInMemory > 0) {
        *des++ = color;
        SizeInMemory -= 4;
    }
    ASSERT_DX(sm_tex->UnlockRect(0));
    ASSERT_DX(g_D3DD->CreateTexture(sx, sy, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_Tex, 0));
    ASSERT_DX(sm_tex->AddDirtyRect(NULL));
    ASSERT_DX(g_D3DD->UpdateTexture(sm_tex, m_Tex));
    // m_Tex->SetPriority(0xFFFFFFFF);

    sm_tex->Release();
}

// //////////////////////////////////////////////////////////////////////////////

#ifndef USE_DX_MANAGED_TEXTURES

void UnloadTextureManaged(DWORD user) {
    CTextureManaged *tex = (CTextureManaged *)user;
    tex->Unload();
}
#endif

// #ifdef _DEBUG

void CTextureManaged::Init(bool to16) {
#ifdef USE_DX_MANAGED_TEXTURES
    if (m_Tex)
        return;
    m_Tex = LoadTextureFromFile(to16, D3DPOOL_MANAGED);
#if defined(MATRIXGAME_BGFX)
    if (!m_Tex) m_Tex = new IDirect3DTexture9();
    if (m_Tex->width == 0) { m_Tex->width = 1; m_Tex->height = 1; }
#endif

    D3DSURFACE_DESC desc;
#if defined(MATRIXGAME_BGFX)
    std::memset(&desc, 0, sizeof(desc));
#endif
    m_Tex->GetLevelDesc(0, &desc);
#if defined(MATRIXGAME_BGFX)
    if (desc.Width == 0) desc.Width = m_Tex->width ? m_Tex->width : 1;
    if (desc.Height == 0) desc.Height = m_Tex->height ? m_Tex->height : 1;
#endif
    m_Size.x = desc.Width;
    m_Size.y = desc.Height;

#else
    if (m_TexFrom)
        return;
    m_TexFrom = LoadTextureFromFile(to16, D3DPOOL_SYSTEMMEM);
#if defined(MATRIXGAME_BGFX)
    if (!m_TexFrom) m_TexFrom = new IDirect3DTexture9();
    if (m_TexFrom->width == 0) { m_TexFrom->width = 1; m_TexFrom->height = 1; }
#endif

    D3DSURFACE_DESC desc;
#if defined(MATRIXGAME_BGFX)
    std::memset(&desc, 0, sizeof(desc));
#endif
    m_TexFrom->GetLevelDesc(0, &desc);
#if defined(MATRIXGAME_BGFX)
    if (desc.Width == 0) desc.Width = m_TexFrom->width ? m_TexFrom->width : 1;
    if (desc.Height == 0) desc.Height = m_TexFrom->height ? m_TexFrom->height : 1;
#endif
    m_Format = desc.Format;
    m_Levelcnt = m_TexFrom->GetLevelCount();
    m_Size.x = desc.Width;
    m_Size.y = desc.Height;

#endif
}

void CTextureManaged::Load(void) {
    DTRACE();

#ifdef USE_DX_MANAGED_TEXTURES
    if (m_Tex == NULL) {
#ifdef _DEBUG
        if (m_Name.length() == 0)
            ERROR_E;
#endif
        Init(false);
    }

#else
    ASSERT(m_Tex == NULL);

    if (m_TexFrom == NULL) {
#ifdef _DEBUG
        if (m_Name.length() == 0)
            ERROR_E;
#endif
        Init(false);
    }

    if (g_D3DD->GetAvailableTextureMem() < (2 * 1024 * 1024)) {
        m_OOM_counter = OOM_TEXTRUE_HIT_COUNTER;
        return;
    }

    // if (m_Tex) { m_Tex->Release(); /*g_TexManagedCnt--;*/ }
    if (FAILED(g_D3DD->CreateTexture(GetSizeX(), GetSizeY(), m_Levelcnt, 0, m_Format, D3DPOOL_DEFAULT, &m_Tex, 0))) {
        m_Tex = NULL;

        m_OOM_counter = OOM_TEXTRUE_HIT_COUNTER;
    }
    else {
        ASSERT_DX(m_TexFrom->AddDirtyRect(NULL));
        ASSERT_DX(g_D3DD->UpdateTexture(m_TexFrom, m_Tex));

        // g_TexManagedCnt++;
    }
#endif

    //    CDText::T("texcnt", g_TexManagedCnt);
}

void CTextureManaged::Unload(void) {
#ifndef USE_DX_MANAGED_TEXTURES
    DTRACE();
    if (m_Tex) {
        m_Tex->Release();
        m_Tex = NULL; /*g_TexManagedCnt--;*/
    }
#endif
    // CDText::T("texcnt", g_TexManagedCnt);
}

// void CTextureManaged::LoadFromBitmapAsIs(const CBitmap & bm)

void CTextureManaged::LoadFromBitmap(int level, const CBitmap &bm, bool convert_to_16bit) {
    DTRACE();

    // #ifdef _DEBUG

    int lx, ly;
    D3DLOCKED_RECT lr;

    lx = bm.SizeX();
    ly = bm.SizeY();

    if (bm.BytePP() == 4) {
        if (convert_to_16bit) {
#ifdef USE_DX_MANAGED_TEXTURES
            ASSERT_DX(m_Tex->LockRect(level, &lr, NULL, 0));
#else
            ASSERT_DX(m_TexFrom->LockRect(level, &lr, NULL, 0));
#endif

            //            m_SizeInMemory+=lr.Pitch*ly;

            const BYTE *sou = bm.Data();
            BYTE *des = (BYTE *)lr.pBits;
            for (int y = 0; y < ly; y++, sou += bm.Pitch() - lx * 4, des += lr.Pitch - lx * 2) {
                for (int x = 0; x < lx; x++, sou += 4, des += 2) {
                    *(des) = (*(sou + 1) & 0xF0) | (*(sou + 2) >> 4);
                    *(des + 1) = (*(sou + 3) & 0xF0) | (*(sou + 0) >> 4);
                }
            }
        }
        else {
#ifdef USE_DX_MANAGED_TEXTURES
            ASSERT_DX(m_Tex->LockRect(level, &lr, NULL, 0));
#else
            ASSERT_DX(m_TexFrom->LockRect(level, &lr, NULL, 0));
#endif
            //            m_SizeInMemory+=lr.Pitch*ly;

            BYTE *sou = (BYTE *)bm.Data();
            BYTE *des = (BYTE *)lr.pBits;
            for (int y = 0; y < ly; y++, sou += bm.Pitch() - lx * 4, des += lr.Pitch - lx * 4) {
                for (int x = 0; x < lx; x++, sou += 4, des += 4) {
                    *(des) = *(sou + 2);
                    *(des + 1) = *(sou + 1);
                    *(des + 2) = *(sou + 0);
                    *(des + 3) = *(sou + 3);
                }
            }

            // undone
        }
    }
    else if (bm.BytePP() == 3) {
        if (convert_to_16bit) {
#ifdef USE_DX_MANAGED_TEXTURES
            ASSERT_DX(m_Tex->LockRect(level, &lr, NULL, 0));
#else
            ASSERT_DX(m_TexFrom->LockRect(level, &lr, NULL, 0));
#endif

            //            m_SizeInMemory+=lr.Pitch*ly;

            BYTE *sou = (BYTE *)bm.Data();
            BYTE *des = (BYTE *)lr.pBits;
            for (int y = 0; y < ly; y++, sou += bm.Pitch() - lx * 3, des += lr.Pitch - lx * 2) {
                for (int x = 0; x < lx; x++, sou += 3, des += 2) {
                    // *(des)=*(sou+2); // r

                    WORD c = ((*(sou + 0) << 7) & 0x7C00) | ((*(sou + 1) << 2) & 0x03E0) | ((*(sou + 2) >> 3) & 0x001F);

                    *((WORD *)des) = c;
                }
            }
        }
        else {
#ifdef USE_DX_MANAGED_TEXTURES
            ASSERT_DX(m_Tex->LockRect(level, &lr, NULL, 0));
#else
            ASSERT_DX(m_TexFrom->LockRect(level, &lr, NULL, 0));
#endif
            //            m_SizeInMemory+=lr.Pitch*ly;

            BYTE *sou = (BYTE *)bm.Data();
            BYTE *des = (BYTE *)lr.pBits;
            for (int y = 0; y < ly; y++, sou += bm.Pitch() - lx * 3, des += lr.Pitch - lx * 4) {
                for (int x = 0; x < lx; x++, sou += 3, des += 4) {
                    *(des) = *(sou + 2);
                    *(des + 1) = *(sou + 1);
                    *(des + 2) = *(sou + 0);
                    *(des + 3) = 255;
                }
            }
        }
    }
    else
        ERROR_E;

#ifdef USE_DX_MANAGED_TEXTURES
    ASSERT_DX(m_Tex->UnlockRect(level));
#else
    ASSERT_DX(m_TexFrom->UnlockRect(level));
#endif
}

void CTextureManaged::LoadFromBitmap(const CBitmap &bm, D3DFORMAT fmt, int levels) {
#ifdef USE_DX_MANAGED_TEXTURES
    if (m_Tex)
        m_Tex->Release();
    m_Tex = NULL;
#else
    Unload();
    if (m_TexFrom)
        m_TexFrom->Release();
#endif

#if defined(MATRIXGAME_BGFX)
    if (bm.Data() && bm.SizeX() > 0 && bm.SizeY() > 0) {
        auto* tex = new IDirect3DTexture9();
        tex->width = bm.SizeX();
        tex->height = bm.SizeY();
        tex->rgba.resize(tex->width * tex->height * 4);
        const uint8_t* src = static_cast<const uint8_t*>(bm.Data());
        for (int y = 0; y < bm.SizeY(); ++y) {
            for (int x = 0; x < bm.SizeX(); ++x) {
                const uint8_t* p = src + y * bm.Pitch() + x * std::max(1, bm.BytePP());
                uint8_t* d = tex->rgba.data() + (y * tex->width + x) * 4;
                if (bm.BytePP() >= 3) { d[0]=p[0]; d[1]=p[1]; d[2]=p[2]; d[3]= bm.BytePP()>=4 ? p[3] : 255; }
                else { d[0]=d[1]=d[2]=p[0]; d[3]=255; }
            }
        }
        mgfx_texture_upload(tex);
#ifdef USE_DX_MANAGED_TEXTURES
        m_Tex = tex;
        m_Size.x = tex->width;
        m_Size.y = tex->height;
#else
        m_TexFrom = tex;
        m_Levelcnt = 1;
        m_Size.x = tex->width;
        m_Size.y = tex->height;
#endif
        return;
    }
#endif

    CBuf buf;

    bm.SaveInDDSUncompressed(buf);

#ifdef USE_DX_MANAGED_TEXTURES
    D3DXCreateTextureFromFileInMemoryEx(g_D3DD, buf.Get(), buf.Len(), 0, 0, levels, 0, fmt, D3DPOOL_MANAGED,
                                        D3DX_FILTER_NONE, D3DX_FILTER_BOX, 0, NULL, NULL, &m_Tex);
#else
    D3DXCreateTextureFromFileInMemoryEx(g_D3DD, buf.Get(), buf.Len(), 0, 0, levels, 0, fmt, D3DPOOL_SYSTEMMEM,
                                        D3DX_FILTER_NONE, D3DX_FILTER_BOX, 0, NULL, NULL, &m_TexFrom);
#endif
    // m_TexFrom->GenerateMipSubLevels(

#ifdef USE_DX_MANAGED_TEXTURES
    if (m_Tex != NULL) {
        D3DSURFACE_DESC desc;
        m_Tex->GetLevelDesc(0, &desc);
        m_Size.x = desc.Width;
        m_Size.y = desc.Height;
    }
#else
    if (m_TexFrom != NULL) {
        m_Levelcnt = m_TexFrom->GetLevelCount();
        D3DSURFACE_DESC desc;
        m_TexFrom->GetLevelDesc(0, &desc);

        m_Format = desc.Format;
        m_Size.x = desc.Width;
        m_Size.y = desc.Height;
    }
#endif
}

void CTextureManaged::LoadFromBitmap(const CBitmap &bm, bool convert_to_16bit, int levelcnt) {
    DTRACE();

#ifdef USE_DX_MANAGED_TEXTURES
    if (m_Tex)
        m_Tex->Release();
    m_Tex = NULL;
#else
    Unload();
    if (m_TexFrom)
        m_TexFrom->Release();
#endif

    int lx, ly;
    D3DLOCKED_RECT lr;

    lx = bm.SizeX();
    ly = bm.SizeY();
    m_Size.x = lx;
    m_Size.y = ly;
    //    m_SizeInMemory=0;

    if (bm.BytePP() == 4) {
        if (convert_to_16bit) {
#ifdef USE_DX_MANAGED_TEXTURES
            if (D3D_OK != (g_D3DD->CreateTexture(lx, ly, levelcnt, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, &m_Tex, 0))) {
                m_Tex = NULL;
                return;
            }
#else
            if (D3D_OK !=
                (g_D3DD->CreateTexture(lx, ly, levelcnt, 0, D3DFMT_A4R4G4B4, D3DPOOL_SYSTEMMEM, &m_TexFrom, 0))) {
                m_TexFrom = NULL;
                return;
            }
            m_Format = D3DFMT_A4R4G4B4;
            m_Levelcnt = m_TexFrom->GetLevelCount();
#endif
        }
        else {
#ifdef USE_DX_MANAGED_TEXTURES
            if (D3D_OK != (g_D3DD->CreateTexture(lx, ly, levelcnt, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_Tex, 0))) {
                m_Tex = NULL;
                return;
            }
#else
            if (D3D_OK !=
                (g_D3DD->CreateTexture(lx, ly, levelcnt, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_TexFrom, 0))) {
                m_TexFrom = NULL;
                return;
            }
            m_Format = D3DFMT_A8R8G8B8;
            m_Levelcnt = m_TexFrom->GetLevelCount();
#endif
        }
    }
    else if (bm.BytePP() == 3) {
        if (convert_to_16bit) {
#ifdef USE_DX_MANAGED_TEXTURES
            if (D3D_OK != (g_D3DD->CreateTexture(lx, ly, levelcnt, 0, D3DFMT_X1R5G5B5, D3DPOOL_MANAGED, &m_Tex, 0))) {
                m_Tex = NULL;
                return;
            }
#else
            if (D3D_OK !=
                (g_D3DD->CreateTexture(lx, ly, levelcnt, 0, D3DFMT_X1R5G5B5, D3DPOOL_SYSTEMMEM, &m_TexFrom, 0))) {
                m_TexFrom = NULL;
                return;
            }
            m_Format = D3DFMT_X1R5G5B5;
            m_Levelcnt = m_TexFrom->GetLevelCount();
#endif
        }
        else {
#ifdef USE_DX_MANAGED_TEXTURES
            if (D3D_OK != (g_D3DD->CreateTexture(lx, ly, levelcnt, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_Tex, 0))) {
                m_Tex = NULL;
                return;
            }
#else
            if (D3D_OK !=
                (g_D3DD->CreateTexture(lx, ly, levelcnt, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_TexFrom, 0))) {
                m_TexFrom = NULL;
                return;
            }
            m_Format = D3DFMT_A8R8G8B8;
            m_Levelcnt = m_TexFrom->GetLevelCount();
#endif
        }
    }
    else if (bm.BytePP() == 1) {
#ifdef USE_DX_MANAGED_TEXTURES
        if (D3D_OK == g_D3DD->CreateTexture(lx, ly, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &m_Tex, 0)) {
            ASSERT_DX(m_Tex->LockRect(0, &lr, NULL, 0));
#else
        if (D3D_OK == g_D3DD->CreateTexture(lx, ly, 1, 0, D3DFMT_L8, D3DPOOL_SYSTEMMEM, &m_TexFrom, 0)) {
            m_Format = D3DFMT_L8;
            ASSERT_DX(m_TexFrom->LockRect(0, &lr, NULL, 0));
#endif
            //            m_SizeInMemory+=lr.Pitch*ly;
            BYTE *sou = (BYTE *)bm.Data();
            BYTE *des = (BYTE *)lr.pBits;
            for (int y = 0; y < ly; y++, sou += bm.Pitch(), des += lr.Pitch) {
                memcpy(des, sou, lx);
            }
#ifndef USE_DX_MANAGED_TEXTURES
            m_Levelcnt = 1;
#endif
            UnlockRect();
#ifdef USE_DX_MANAGED_TEXTURES
        }
        else if (D3D_OK == g_D3DD->CreateTexture(lx, ly, 1, 0, D3DFMT_A8L8, D3DPOOL_MANAGED, &m_Tex, 0)) {
            ASSERT_DX(m_Tex->LockRect(0, &lr, NULL, 0));
#else
        }
        else if (D3D_OK == g_D3DD->CreateTexture(lx, ly, 1, 0, D3DFMT_A8L8, D3DPOOL_SYSTEMMEM, &m_TexFrom, 0)) {
            m_Format = D3DFMT_A8L8;
            ASSERT_DX(m_TexFrom->LockRect(0, &lr, NULL, 0));
#endif
            //            m_SizeInMemory+=lr.Pitch*ly;
            BYTE *sou = (BYTE *)bm.Data();
            BYTE *des = (BYTE *)lr.pBits;
            for (int y = 0; y < ly; y++, sou += bm.Pitch(), des += lr.Pitch) {
                BYTE *sou1 = sou;
                WORD *des1 = (WORD *)des;
                int lx2 = lx;
                while (lx2-- > 0) {
                    WORD out = (0xFF << 8) | (*sou1 << 0);
                    *des1 = out;
                    ++sou1;
                    ++des1;
                }
            }
#ifndef USE_DX_MANAGED_TEXTURES
            m_Levelcnt = 1;
#endif
            UnlockRect();
        }
        else {
#ifdef USE_DX_MANAGED_TEXTURES
            if (D3D_OK != (g_D3DD->CreateTexture(lx, ly, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &m_Tex, 0))) {
                m_Tex = NULL;
                return;
            }
            ASSERT_DX(m_Tex->LockRect(0, &lr, NULL, 0));
#else
            if (D3D_OK != (g_D3DD->CreateTexture(lx, ly, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &m_TexFrom, 0))) {
                m_TexFrom = NULL;
                return;
            }
            m_Format = D3DFMT_X8R8G8B8;
            ASSERT_DX(m_TexFrom->LockRect(0, &lr, NULL, 0));
#endif
            //            m_SizeInMemory+=lr.Pitch*ly;
            BYTE *sou = (BYTE *)bm.Data();
            BYTE *des = (BYTE *)lr.pBits;
            for (int y = 0; y < ly; y++, sou += bm.Pitch(), des += lr.Pitch) {
                BYTE *sou1 = sou;
                DWORD *des1 = (DWORD *)des;
                int lx2 = lx;
                while (lx2-- > 0) {
                    DWORD out = (0xFF << 24) | (*sou1 << 16) | (*sou1 << 8) | (*sou1 << 0);
                    *des1 = out;
                    ++sou1;
                    ++des1;
                }
            }
#ifndef USE_DX_MANAGED_TEXTURES
            m_Levelcnt = 1;
#endif
            UnlockRect();
        }

        return;
    }
    else
        ERROR_E;

    LoadFromBitmap(0, bm, convert_to_16bit);

#ifdef USE_DX_MANAGED_TEXTURES
    int lcnt = m_Tex->GetLevelCount();
#else
    int lcnt = m_TexFrom->GetLevelCount();
#endif
    CBitmap bc;

    if (bm.BytePP() == 4)
        bc.CreateRGBA(lx, ly);
    if (bm.BytePP() == 3)
        bc.CreateRGB(lx, ly);
    bc.Copy(CPoint(0, 0), bm.Size(), bm, CPoint(0, 0));

    for (int i = 1; i < lcnt; ++i) {
        bc.Make2xSmaller();
        LoadFromBitmap(i, bc, convert_to_16bit);
    }
}

void CTextureManaged::Ramka(int x, int y, int w, int h, DWORD c) {
    D3DLOCKED_RECT lr;
#ifdef _DEBUG
    ASSERT(!FLAG(g_Flags, GFLAG_RENDERINPROGRESS));
#endif
    RECT r;
    r.left = x;
    r.top = y;
    r.right = x + w;
    r.bottom = y + h;
#ifdef USE_DX_MANAGED_TEXTURES
    ASSERT_DX(m_Tex->LockRect(0, &lr, &r, 0));
#else
    ASSERT_DX(m_TexFrom->LockRect(0, &lr, &r, 0));
#endif
    DWORD *src = (DWORD *)lr.pBits;

    DWORD *src_0 = src + 1;
    DWORD *src_1 = (DWORD *)(((BYTE *)src) + lr.Pitch * (h - 1)) + 1;

    --w;
    while (h > 0) {
        *src = c;
        *(src + w) = c;
        src = (DWORD *)(((BYTE *)src) + lr.Pitch);
        --h;
    }

    while (w > 0) {
        *src_0 = c;
        *src_1 = c;
        ++src_0;
        ++src_1;
        --w;
    }

#ifndef USE_DX_MANAGED_TEXTURES
    if (m_Tex) {
        m_Tex->Release();
        m_Tex = NULL;
    }
#endif

    UnlockRect();
}

DWORD CTextureManaged::GetPixelColor(int x, int y) {
    D3DLOCKED_RECT lr;
#ifdef _DEBUG
    ASSERT(!FLAG(g_Flags, GFLAG_RENDERINPROGRESS));
#endif
    RECT r;
    r.left = x;
    r.top = y;
    r.right = x + 1;
    r.bottom = y + 1;

    if (x < 0 || y < 0 || x >= m_Size.x || y >= m_Size.y)
        return 0;

#ifdef USE_DX_MANAGED_TEXTURES
    ASSERT_DX(m_Tex->LockRect(0, &lr, &r, D3DLOCK_READONLY));
#else
    ASSERT_DX(m_TexFrom->LockRect(0, &lr, &r, D3DLOCK_READONLY));
#endif
    BYTE *src = (BYTE *)lr.pBits;

    // DWORD c = *(DWORD *)(src + (lr.Pitch * y) + x * 4);
    DWORD c = *(DWORD *)(src);

    UnlockRect();
    return c;
}
