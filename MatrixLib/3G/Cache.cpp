// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "3g.hpp"

#include "Cache.hpp"
#include "CFile.hpp"
#include "VectorObject.hpp"

#include <utils.hpp>

#include <fstream>
#include <filesystem>
#if !defined(_WIN32)
#include <execinfo.h>
#endif

void CacheReplaceFileExt(
    std::wstring &outname,
    const std::wstring_view mname,
    const std::wstring_view ext)
{
    DTRACE();

    outname.assign(mname.begin(), mname.end());
    auto dot = outname.find_last_of(L"./\\");
    if (dot != std::wstring::npos && outname[dot] == L'.') outname.resize(dot);
    outname += ext;

    // Append the part after the '?' (if it exists)
    size_t lenfile = mname.find(L'?');
    if (lenfile != std::wstring_view::npos)
    {
        outname += mname.substr(lenfile);
    }
}

void CacheReplaceFileNameAndExt(
    std::wstring &outname,
    const std::wstring_view mname,
    const std::wstring_view replname)
{
    DTRACE();

    outname.assign(replname.begin(), replname.end());
    auto slash = std::wstring(mname).find_last_of(L"/\\");
    if (slash != std::wstring::npos) outname.insert(0, mname.substr(0, slash + 1));

    // Append the part after the '?' (if it exists)
    size_t lenfile = mname.find(L'?');
    if (lenfile != std::wstring_view::npos)
    {
        outname += mname.substr(lenfile);
    }
}

// //////////////////////////////////////////////////////////////////////////////

void CCacheData::Prepare()
{
    DTRACE();
    if (!IsLoaded())
    {
        Load();
    }
    if (m_Cache)
    {
        m_Cache->Up(this);
    }
}

void CCacheData::LoadFromFile(CBuf &buf, const wchar *exts) {
    DTRACE();

    std::wstring tstr, tname;

    tname = ParamParser{m_Name}.GetStrPar(0, L"?");

    if (!CFile::FileExist(tstr, tname.c_str(), exts, false)) {
        std::fprintf(stderr, "cache miss name=%ls exts=%ls\n", m_Name.c_str(), exts ? exts : L"");
#if !defined(_WIN32)
        void* bt[12]; int n = backtrace(bt, 12); backtrace_symbols_fd(bt, n, 2);
#endif
        ERROR_S(utils::format(L"File not found: %ls   Exts: %ls", tname.c_str(), exts));
    }

    buf.LoadFromFile(tstr);

    /*	ASSERT(m_Name.Len()>0);

        int len=m_Name.Len();
        wchar * str=m_Name.c_str();

        int lenfile=0; while(lenfile<len && str[lenfile]!='?') lenfile++;
        ASSERT(lenfile>0);

        CFile fi(g_CacheHeap);
        fi.Init(str,lenfile);
        if(fi.OpenReadNE()) {
            buf.Len(fi.Size());
            fi.Read(buf.c_str(),buf.Len());
            buf.Pointer(0);
            return;
        }

        WIN32_FIND_DATA fd;
        HANDLE fh=FindFirstFile(utils::from_wstring(std::wstring(str,lenfile,g_CacheHeap)+L".*").c_str()).c_str(),&fd);
        if(fh==INVALID_HANDLE_VALUE) ERROR_S2(L"File not found: ",m_Name.c_str());
        FindClose(fh);

        int lenpath=lenfile; while(lenpath>0 && str[lenpath-1]!='\\' && str[lenpath-1]!='/') lenpath--;

        if(lenpath>0) fi.Init(std::wstring(str,lenpath)+std::wstring(fd.cFileName),g_CacheHeap));
        else fi.Init(std::wstring(fd.cFileName),g_CacheHeap));

        if(fi.OpenReadNE()) {
            buf.Len(fi.Size());
            fi.Read(buf.c_str(),buf.Len());
            buf.Pointer(0);
            return;
        }

        ERROR_S2(L"Error open file: ",m_Name.c_str());*/
}

// //////////////////////////////////////////////////////////////////////////////

void CCache::Add(CCacheData *cd)
{
    DTRACE();

    ASSERT(cd);

    _data.push_back(cd);
    cd->m_Cache = this;
}

void CCache::Delete(CCacheData *cd)
{
    DTRACE();

    ASSERT(cd);
    ASSERT(cd->m_Cache == this);

    _data.remove(cd);
    cd->m_Cache = NULL;
}

void CCache::Up(CCacheData *cd)
{
    DTRACE();

    ASSERT(cd);
    ASSERT(cd->m_Cache == this);

    _data.remove(cd);
    _data.push_front(cd);
}

void CCache::PreLoad(void)
{
    for (auto cd : _data)
    {
        if (cd->m_Type == CacheClass::VO)
        {
            if (!cd->IsLoaded())
                cd->Load();
        }
    }
}

CCacheData* CCache::Find(CacheClass cc, const std::wstring_view name)
{
    DTRACE();

    for (auto cd : _data)
    {
        if (cd->m_Type == cc && cd->m_Name == name)
        {
            return cd;
        }
    }
    return NULL;
}

CCacheData* CCache::Get(CacheClass cc, const std::wstring_view name)
{
    DTRACE();

    CCacheData *cd = Find(cc, name);
    if (cd)
    {
        return cd;
    }

#ifdef _DEBUG
    cd = Create(cc, __FILE__, __LINE__);
#else
    cd = Create(cc);
#endif
    cd->m_Name = name;
    Add(cd);

    return cd;
}

#ifdef _DEBUG
static std::list<CCacheData*> _debug_data;
CCacheData *CCache::Create(CacheClass cc, const char *file, int line)
#else
CCacheData *CCache::Create(CacheClass cc)
#endif
{
    DTRACE();

    CCacheData *el;

    switch (cc) {
        case CacheClass::Texture: {
            el = HNew(g_CacheHeap) CTexture;
            break;
        }
        case CacheClass::TextureManaged: {
            el = HNew(g_CacheHeap) CTextureManaged;
            break;
        }
        case CacheClass::VO: {
            el = HNew(g_CacheHeap) CVectorObject;
            break;
        }
        default:
            ERROR_E;
    }

#ifdef _DEBUG
    el->d_file = file;
    el->d_line = line;
    _debug_data.push_back(el);
#endif
    return el;
}

void CCache::Destroy(CCacheData *cd)
{
    DTRACE();

    ASSERT(cd);

#ifdef _DEBUG
    _debug_data.remove(cd);
#endif

    HDelete(CCacheData, cd, g_CacheHeap);
}

#ifdef _DEBUG
void CCache::Dump(void) {
    std::string buf{"Cache Dump\n"};
    int cnt = 0;
    for (auto cd : _debug_data)
    {
        ++cnt;
        const char* type = "Unknown       ";

        if (cd->m_Type == CacheClass::Texture)
            type = "Texture       ";
        if (cd->m_Type == CacheClass::TextureManaged)
            type = "TextureManaged";
        if (cd->m_Type == CacheClass::VO)
            type = "Object        ";

        std::string name{(cd->m_Name.empty()) ? std::string{"NULL"} : utils::from_wstring(cd->m_Name.c_str())};

        const char* loaded;
        if (cd->IsLoaded())
            loaded = "+";
        else
            loaded = "-";

        if (buf.length() < 65000) {
            buf += utils::format("%s%s : %s (%s : %i)\n", loaded, type, name.c_str(), cd->d_file, cd->d_line);
        }
    }

    buf += utils::format("count: %i\n", cnt);

    // MessageBox(g_Wnd, buf, "D3D Dump", MB_ICONINFORMATION);

    std::ofstream out("debug_dump.txt");
    out << buf;
}
#endif

void CCache::Clear(void)
{
    for (auto it = _data.begin(); it != _data.end();)
    {
        auto cd = *it;
        it++;

        Delete(cd);
        CCache::Destroy(cd);
    }

    _data.clear();
}

// //////////////////////////////////////////////////////////////////////////////
CCache *g_Cache;
CHeap *g_CacheHeap;
const wchar *CacheExtsTex = L"png~jpg~bmp~dds~strg";

void CacheInit(void) {
    DTRACE();

    g_CacheHeap = HNew(NULL) CHeap();
    g_Cache = HNew(g_CacheHeap) CCache;
}

void CacheDeinit() {
    DTRACE();

    if (g_Cache) {
        ASSERT(g_CacheHeap);
        g_Cache->Clear();
        HDelete(CCache, g_Cache, g_CacheHeap);
        g_Cache = NULL;
    }

    if (g_CacheHeap) {
        HDelete(CHeap, g_CacheHeap, NULL);
        g_CacheHeap = NULL;
    }
}
