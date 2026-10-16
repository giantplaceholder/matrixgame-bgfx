// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include <new>

#include "MatrixMapTexture.hpp"
#include "MatrixMap.hpp"
#include "MatrixLoadProgress.hpp"
#include "MatrixTerSurface.hpp"

CBottomTextureUnion *CBottomTextureUnion::m_Textures;
int CBottomTextureUnion::m_TexturesCount;

// static void FreeBottomTextureUnionHandler(DWORD, DWORD user)
//{
//    DTRACE();
//    CBottomTextureUnion * t= (CBottomTextureUnion *)user;
//    t->Unload();
//}

void CBottomTextureUnion::MakeFromBitmap(const CBitmap &bm) {
    m_Texture = CACHE_CREATE_TEXTUREMANAGED();
    // m_Texture->LoadFromBitmap(bm, D3DFMT_DXT1, 6);
    m_Texture->LoadFromBitmap(bm, D3DFMT_DXT1, 6);
}

CBottomTextureUnion::~CBottomTextureUnion(void) {
    DTRACE();
    if (m_Texture)
        CCache::Destroy(m_Texture);
}

void CBottomTextureUnion::Init(int n) {
    Clear();
    m_TexturesCount = n;
    m_Textures = (CBottomTextureUnion *)HAlloc(sizeof(CBottomTextureUnion) * n, g_MatrixHeap);
    for (int i = 0; i < n; ++i) {
        new(&m_Textures[i]) CBottomTextureUnion();
    }
}

void CBottomTextureUnion::Clear(void) {
    if (m_Textures) {
        for (int i = 0; i < m_TexturesCount; ++i) {
            m_Textures[i].~CBottomTextureUnion();
        }
        HFree(m_Textures, g_MatrixHeap);
        m_Textures = NULL;
    }
}
