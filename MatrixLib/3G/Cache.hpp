// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include "CMain.hpp"
#include "CHeap.hpp"
#include "CBuf.hpp"

#include <list>

class CCache;

enum class CacheClass
{
    Unknown = 0,
    Texture,
    TextureManaged,
    VO
};

class CCacheData
{
public:
    CCache *m_Cache{nullptr};
#ifdef _DEBUG
    const char *d_file;
    int d_line;
#endif

    CacheClass m_Type{CacheClass::Unknown};
    std::wstring m_Name{};

    int m_Ref{0};  // Кол-во ссылок на эти данные. (Для временных данных)

public:
    CCacheData(void) = default;
    virtual ~CCacheData() = default;

    void RefInc(void) { m_Ref++; }
    void RefDec(void) {
        m_Ref--;
        ASSERT(m_Ref >= 0);
    }
    int Ref(void) { return m_Ref; }
    bool RefDecUnload(void) {
        m_Ref--;
        if (m_Ref <= 0) {
            Unload();
            return true;
        }
        return false;
    }

    void Prepare(void);

    void LoadFromFile(CBuf &buf, const wchar *exts = NULL);

    virtual bool IsLoaded(void) = 0;
    virtual void Unload(void) = 0;
    virtual void Load(void) = 0;
};

class CCache
{
public:
    std::list<CCacheData*> _data;

public:
    CCache() = default;
    ~CCache() = default;

#ifdef _DEBUG
    static void Dump(void);
#endif

    void Add(CCacheData *cd);
    void Delete(CCacheData *cd);
    void Up(CCacheData *cd);

    void PreLoad(void);

    CCacheData *Find(CacheClass cc, const std::wstring_view name);
    CCacheData *Get(CacheClass cc, const std::wstring_view name);
#ifdef _DEBUG
    static CCacheData *Create(CacheClass cc, const char *file, int line);
#else
    static CCacheData *Create(CacheClass cc);
#endif
    static void Destroy(CCacheData *cd);

    void Clear(void);
};

#ifdef _DEBUG
#define CACHE_CREATE_VO()             (CVectorObject *)CCache::Create(CacheClass::VO, __FILE__, __LINE__)
#define CACHE_CREATE_TEXTURE()        (CTexture *)CCache::Create(CacheClass::Texture, __FILE__, __LINE__)
#define CACHE_CREATE_TEXTUREMANAGED() (CTextureManaged *)CCache::Create(CacheClass::TextureManaged, __FILE__, __LINE__)
#else
#define CACHE_CREATE_VO()             (CVectorObject *)CCache::Create(CacheClass::VO)
#define CACHE_CREATE_TEXTURE()        (CTexture *)CCache::Create(CacheClass::Texture)
#define CACHE_CREATE_TEXTUREMANAGED() (CTextureManaged *)CCache::Create(CacheClass::TextureManaged)
#endif

extern CCache *g_Cache;
extern CHeap *g_CacheHeap;
extern const wchar *CacheExtsTex;

void CacheInit(void);
void CacheDeinit(void);

void CacheReplaceFileExt(std::wstring &outname, const std::wstring_view mname, const std::wstring_view ext);
void CacheReplaceFileNameAndExt(std::wstring &outname, const std::wstring_view mname, const std::wstring_view replname);
