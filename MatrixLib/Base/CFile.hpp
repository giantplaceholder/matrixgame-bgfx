// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include "CMain.hpp"

#include <windows.h>
#include <string>

namespace Base {

#ifndef MAXEXP_EXPORTS
class CPackCollection;
#endif

typedef void (*ENUM_FILES)(const std::wstring &name, DWORD user);

class BASE_API CFile : public CMain {
#ifndef MAXEXP_EXPORTS
    static CPackCollection *m_Packs;
    static int m_PacksRef;

    DWORD m_PackHandle;
#endif

    HANDLE m_Handle;   // Handle файла
    std::wstring m_FileName;  // Имя файла
    int m_Open;        // Кол-во вызовов Open

public:
    CFile();
    CFile(const std::wstring &filename);
    CFile(const wchar *filename);
    CFile(const wchar *filename, int len);
    ~CFile();

    static void StaticInit(void) {
#ifndef MAXEXP_EXPORTS
        m_Packs = NULL;
        m_PacksRef = 0;
#endif
    }

#ifndef MAXEXP_EXPORTS
    static void AddPackFile(const wchar *name);
    static void OpenPackFiles(void);
    static void ReleasePackFiles(void);
    static bool PackFilesReady(void) { return m_Packs != nullptr; }
    static void ListPackFiles(int limit);
#endif

    static void FindFiles(const std::wstring &folderfrom, const wchar *files, ENUM_FILES ef, uintptr_t user);

    void Clear(void);

    void Init(const wchar *filename, int len);
    void Init(const wchar *filename) { Init(filename, std::wcslen(filename)); }
    void Init(const std::wstring &filename) { Init(filename.c_str(), filename.length()); }

    bool IsOpen(void) { return m_Open > 0; }

    void Open(DWORD shareMode = 0);                    // FILE_SHARE_DELETE FILE_SHARE_READ FILE_SHARE_WRITE
    void OpenRead(DWORD shareMode = FILE_SHARE_READ);  // FILE_SHARE_DELETE FILE_SHARE_READ FILE_SHARE_WRITE
    bool OpenReadNE(DWORD shareMode = FILE_SHARE_READ);
    void Create(DWORD shareMode = 0);
    bool CreateNE(DWORD shareMode = 0);

    void Close(void);

    DWORD Size(void) const;
    int64_t SizeFull(void) const;

    int64_t PointerFull(void) const;
    void PointerFull(int64_t zn, int from = FILE_BEGIN) const;  // FILE_BEGIN FILE_CURRENT FILE_END

    DWORD Pointer(void) const;
    void Pointer(DWORD zn, int from = FILE_BEGIN) const;  // FILE_BEGIN FILE_CURRENT FILE_END

    void Read(void *buf, DWORD kolbyte);
    void Write(void *buf, DWORD kolbyte);

    static bool FileExist(std::wstring &outname, const wchar *mname, const wchar *exts = NULL, bool withpar = false);
};

}  // namespace Base