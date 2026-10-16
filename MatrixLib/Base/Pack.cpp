// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "Pack.hpp"

#include <utility>
#include <tuple> // for std::tie
#include <cctype> // for std::toupper
#include <algorithm>
#include <cstdio>

#include <utils.hpp>

// #ifdef BLABLA

#undef ZEXPORT
#define ZEXPORT __cdecl

#include "zlib.h"

namespace {

std::pair<std::string, std::string>
split_path(const std::string& path, const std::string& delimiters)
{
    std::string beg, rem;
    auto pos = path.find_first_of(delimiters);

    if (pos != std::string::npos)
    {
        beg = path.substr(0, pos);
        rem = path.substr(pos + 1); // +1 to skip delimiter
    }
    else // if no delimiters were found
    {
        beg = path;
        rem.clear();
    }

    return std::make_pair(beg, rem);
}

std::string rec_name(const char* raw)
{
    if (!raw)
        return {};
    size_t n = 0;
    while (n < MAX_FILENAME_LENGTH && raw[n] != '\0')
        ++n;
    return std::string(raw, n);
}

std::string to_upper(const std::string& str)
{
    std::string res{str};
    for (auto& sym : res)
    {
        unsigned char u = static_cast<unsigned char>(sym);
        if (u >= 'a' && u <= 'z')
            sym = static_cast<char>(u - ('a' - 'A'));
    }
    return res;
}

} // namespace

// #define _MAKESTR1(x) #x
// #define MAKESTR(n) _MAKESTR1(n)

// char *sss2 = MAKESTR(5);

// @rdesc - размер сжатых данных  0 - ошибка

// if desbuf==NULL then return размер выходного буфера
static int OKGF_ZLib_UnCompress(BYTE *desbuf, int lendesbuf, BYTE *soubuf, int lensoubuf) {
    uLongf deslen = lendesbuf;

    if (lensoubuf < 8)
        return 0;
    if (*(soubuf + 0) != 'Z' || *(soubuf + 1) != 'L' || *(soubuf + 2) != '0' || *(soubuf + 3) != '2')
        return 0;
    if (desbuf == NULL)
        return *((DWORD *)(soubuf + 4));
    if (*((DWORD *)(soubuf + 4)) > DWORD(lendesbuf))
        return 0;

    if (uncompress(desbuf, &deslen, soubuf + 8, lensoubuf - 8) != Z_OK)
        return 0;
    if (deslen != *((DWORD *)(soubuf + 4)))
        return 0;
    return deslen;
}

// int OKGF_ZLib_Compress2(BYTE * desbuf,int len_des_buf, BYTE * soubuf,int len_sou_buf,int fSpeed)

// if desbuf==NULL then return размер выходного буфера
int OKGF_ZLib_UnCompress2(BYTE *desbuf, int len_des_buf, BYTE *soubuf, int len_sou_buf) {
    uLongf deslen = len_des_buf;

    if (len_sou_buf < 8)
        return 0;
    if (*(soubuf + 0) != 'Z' || *(soubuf + 1) != 'L' || *(soubuf + 2) != '0' || *(soubuf + 3) != '2')
        return 0;
    //  if(desbuf==NULL) return *((DWORD *)(soubuf+4));
    if (*((DWORD *)(soubuf + 4)) > DWORD(len_des_buf))
        return 0;

    if (uncompress(desbuf, &deslen, soubuf + 8, len_sou_buf - 8) != Z_OK)
        return 0;
    if (deslen != *((DWORD *)(soubuf + 4)))
        return 0;
    return deslen;
}

namespace Base {

// **********************************************************************

CHsFolder::CHsFolder(const std::string &Name, CHeap *heap) : m_Heap(heap), m_Name(Name), m_RealName(Name) {
    m_Files = NULL;
    m_FolderRec.m_Size = sizeof(SFolderRec);
    m_FolderRec.m_Recnum = 0;
    m_FolderRec.m_RecSize = sizeof(SFileRec);
    m_Parent = NULL;
    m_Name = to_upper(m_Name);
}

CHsFolder::CHsFolder(const std::string &Name, CHsFolder *parent, CHeap *heap)
  : m_Heap(heap), m_Name(Name), m_RealName(Name) {
    m_Files = NULL;
    m_FolderRec.m_Size = sizeof(SFolderRec);
    m_FolderRec.m_Recnum = 0;
    m_FolderRec.m_RecSize = sizeof(SFileRec);
    m_Parent = parent;
    m_Name = to_upper(m_Name);
}

SFileRec *CHsFolder::GetFileRec(const std::string& name) const {
    if (!m_Files)
        return NULL;
    if (m_FolderRec.m_Recnum > 1000000)
        return NULL;

    std::string n = to_upper(name);
    for (DWORD i = 0; i < m_FolderRec.m_Recnum; ++i) {
        SFileRec *PFile = GetFileRec(int(i));
        if (!PFile || PFile->m_Free != 0)
            continue;
        std::string stored = rec_name(PFile->m_Name);
        std::string real = rec_name(PFile->m_RealName);
        if (n == to_upper(stored) || n == to_upper(real))
            return PFile;
    }
    return NULL;
}

SFileRec *CHsFolder::GetFreeRec(void) const {
    SFileRec *PFile;

    for (DWORD i = 0; i < m_FolderRec.m_Recnum; ++i) {
        PFile = GetFileRec(i);
        if (PFile->m_Free != 0) {
            return PFile;
        }
    }
    return NULL;
}

// Procedure   CHsFolder.AllocEmptyFolder;

bool CHsFolder::ReadFolder(uintptr_t Handle, uintptr_t Offset) {
    DWORD temp;

    if (m_Files)
        return false;

    // Читаем заголовок папки
    SetFilePointer((HANDLE)Handle, Offset, NULL, FILE_BEGIN);
    BOOL res = ::ReadFile((HANDLE)Handle, &m_FolderRec, sizeof(SFolderRec), &temp, NULL);
    if (res == FALSE || temp != sizeof(SFolderRec))
        return false;
    if (sizeof(SFileRec) != m_FolderRec.m_RecSize) {
        std::fprintf(stderr, "pack record size %u != sizeof(SFileRec) %zu\n", static_cast<unsigned>(m_FolderRec.m_RecSize), sizeof(SFileRec));
        return false;
    }

    // Теперь необходимо прочитать данные, связанные с хранимыми файлами в данной папке
    int sz = m_FolderRec.m_Recnum * m_FolderRec.m_RecSize;
    m_Files = (SFileRec *)HAllocClear(sz, m_Heap);

    res = ::ReadFile((HANDLE)Handle, m_Files, sz, &temp, NULL);
    if (res == FALSE || temp != sz) {
        for (DWORD i = 0; i < m_FolderRec.m_Recnum; ++i) {
            m_Files[i].m_Extra = 0;
        }
        Clear();
        return false;
    }
    for (DWORD i = 0; i < m_FolderRec.m_Recnum; ++i) {
        m_Files[i].m_Extra = 0;
        m_Files[i].m_NType = m_Files[i].m_Type;
    }

    // Вся информация о файлах в текущей директории прочитана
    // Теперь читаем информацию о вложенных папках
    for (DWORD i = 0; i < m_FolderRec.m_Recnum; ++i) {
        if (m_Files[i].m_Type == FILEEC_FOLDER && m_Files[i].m_Free == 0) {
            CHsFolder *PFolder = HNew(m_Heap) CHsFolder(rec_name(m_Files[i].m_RealName), this, m_Heap);
            SetChild(&m_Files[i], PFolder);
            if (!PFolder->ReadFolder(Handle, m_Files[i].m_Offset)) {
                Clear();
                return false;
            }
        }
    }
    return true;
}

void CHsFolder::Clear(void) {
    if (m_Files == NULL)
        return;
    for (DWORD i = 0; i < m_FolderRec.m_Recnum; ++i) {
        if (m_Files[i].m_Type == FILEEC_FOLDER && m_Files[i].m_Free == 0) {
            CHsFolder *PFolder = Child(&m_Files[i]);
            if (PFolder)
                HDelete(CHsFolder, PFolder, m_Heap);
            SetChild(&m_Files[i], nullptr);
        }
    }
    HFree(m_Files, m_Heap);
    m_Files = NULL;
    m_Children.clear();
    m_FolderRec.m_Recnum = 0;
    m_FolderRec.m_Size = sizeof(SFolderRec);
    UpdateFileRec();
}

bool CHsFolder::FileExists(const std::string& name) const {
    std::string beg, rem;
    std::tie(beg, rem) = split_path(name, "/\\");

    SFileRec *PFile = GetFileRec(beg);
    if (PFile == NULL)
        return false;
    if (PFile->m_Type == FILEEC_FOLDER) {
        if (rem.empty())
            return false;
        CHsFolder *PFolder = Child(PFile);
        if (!PFolder) {
            std::fprintf(stderr, "pack folder missing child for '%s'\n", name.c_str());
            return false;
        }
        return PFolder->FileExists(rem);
    }
    return rem.empty();
}

// Procedure   CHsFolder.SetFileType(name:string; NType:DWORD);

// Procedure   CHsFolder.SetFolderType(name:string; NType:DWORD);

bool CHsFolder::PathExists(const std::string& name) const {
    std::string beg, rem;
    std::tie(beg, rem) = split_path(name, "/\\");

    SFileRec *PFile = GetFileRec(beg);
    if (PFile == NULL)
        return false;

    if (PFile->m_Type == FILEEC_FOLDER) {
        if (rem.empty())
            return true;
        CHsFolder *PFolder = Child(PFile);
        if (!PFolder)
            return false;
        return PFolder->PathExists(rem);
    }
    return rem.empty();
}


bool CHsFolder::FindLeaf(const std::string& leaf, std::string& out_path) const {
    std::string want = to_upper(leaf.c_str());
    for (DWORD i = 0; i < m_FolderRec.m_Recnum; ++i) {
        SFileRec* rec = GetFileRec(i);
        if (!rec || rec->m_Free != 0) continue;
        char real[MAX_FILENAME_LENGTH + 1] = {};
        std::memcpy(real, rec->m_RealName, MAX_FILENAME_LENGTH);
        char stored[MAX_FILENAME_LENGTH + 1] = {};
        std::memcpy(stored, rec->m_Name, MAX_FILENAME_LENGTH);
        if (rec->m_Type == FILEEC_FOLDER) {
            CHsFolder* child = Child(rec);
            if (!child) continue;
            std::string nested;
            if (child->FindLeaf(leaf, nested)) {
                out_path = rec_name(real) + "/" + nested;
                return true;
            }
        } else if (want == to_upper(rec_name(real)) || want == to_upper(rec_name(stored))) {
            out_path = rec_name(real);
            return true;
        }
    }
    return false;
}

SFileRec *CHsFolder::GetFileRecEx(const std::string& name) const {
    std::string beg, rem;
    std::tie(beg, rem) = split_path(name, "/\\");

    SFileRec *PFile = GetFileRec(beg);
    if (PFile == nullptr)
        return nullptr;
    if (PFile->m_Type == FILEEC_FOLDER) {
        if (rem.empty())
            return PFile;
        CHsFolder *PFolder = Child(PFile);
        if (!PFolder)
            return nullptr;
        return PFolder->GetFileRecEx(rem);
    }
    return rem.empty() ? PFile : nullptr;
}

// Function    CHsFolder.ReAllocFileRecs(number:integer):boolean;

// ////////////////////////////////////////////////////////////////////////

// Function    CHsFolder.CreateFolder(name:string):boolean;

// Function    CHsFolder.AddPath(name:string):boolean;

std::string CHsFolder::GetFullPath(const std::string& name) {
    if (m_Parent) {
        return m_Parent->GetFullPath(utils::format("%s\\%s", m_RealName.c_str(), name.c_str()));
    }
    else {
        return name;
    }
}

DWORD CHsFolder::CompressedFileSize(DWORD Handle, const std::string& filename) {
    std::string beg, rem;
    std::tie(beg, rem) = split_path(filename, "/\\");

    SFileRec *PFile = GetFileRec(beg);
    if (PFile == NULL)
        return 0xFFFFFFFF;

    // DWORD size,temp;

    if (PFile->m_Type == FILEEC_FOLDER) {
        if (rem.empty())
            return 0xFFFFFFFF;
        CHsFolder *PFolder = Child(PFile);
        if (!PFolder)
            return 0xFFFFFFFF;
        return PFolder->CompressedFileSize(Handle, rem);
    }
    if (!rem.empty())
        return 0xFFFFFFFF;

    // Нашли файл - теперь необходимо вычислить размер этого файла
    // Случай 1 - файл уже упакован и сжат, а его свойства не были изменены внутри пакета
    if (PFile->m_Offset != 0 && PFile->m_Type == FILEEC_COMPRESSED) {
        return PFile->m_Size - 4;
    }

    ERROR_S(L"Error getting compressed file size");

    // // В любом из двух следующих случаев файл следует пережать в памяти

    // totalsize:=0;
}

DWORD CHsFolder::DecompressedFileSize(DWORD Handle, const std::string& filename) {
    std::string beg, rem;
    std::tie(beg, rem) = split_path(filename, "/\\");

    SFileRec *PFile = GetFileRec(beg);
    if (PFile == NULL)
        return 0xFFFFFFFF;

    // size:DWORD;

    if (PFile->m_Type == FILEEC_FOLDER) {
        if (rem.empty())
            return 0xFFFFFFFF;
        CHsFolder *PFolder = Child(PFile);
        return PFolder->DecompressedFileSize(Handle, rem);
    }
    if (!rem.empty())
        return 0xFFFFFFFF;

    // Нашли файл - теперь необходимо вычислить размер этого файла
    // Случай 1 - файл уже упакован

    if (PFile->m_Offset != 0) {
        return PFile->m_RealSize;
    }

    ERROR_S(L"Error getting decompressed file size");

    //    // Случай 2 - файл еще не упакован - ищем его на диске
}

int CHsFolder::GetLocalSize(void) {
    int size = 0;

    for (DWORD i = 0; i < m_FolderRec.m_Recnum; ++i) {
        if (m_Files[i].m_Free == 0) {
            size += m_Files[i].m_Size;
        }
    }
    return size;
}

void CHsFolder::UpdateFileRec(void) {
    if (m_Parent) {
        SFileRec *PFile = m_Parent->GetFileRec(m_RealName);
        if (PFile == NULL)
            ERROR_S(L"Packet file system error!");
        if (PFile->m_Type != FILEEC_FOLDER)
            ERROR_S(L"Name file/folder conflict");
        PFile->m_Size = m_FolderRec.m_Size;
        PFile->m_Offset = 0;
    }
}

// ******** Функция возвращает новое смещение в файле **************//

void CHsFolder::ListFileNames(FILENAME_CALLBACK_FUNC Func) {
    if (!m_RealName.empty()) {
        Func(true, false, m_RealName);
    }
    for (DWORD i = 0; i < m_FolderRec.m_Recnum; ++i) {
        SFileRec *PFile = m_Files + i;
        if (PFile->m_Free == 0) {
            if (PFile->m_Type == FILEEC_FOLDER) {
                CHsFolder *PFolder = Child(PFile);
                if (PFolder == NULL)
                    continue;
                PFolder->ListFileNames(Func);
            }
            else {
                if (PFile->m_NType == FILEEC_COMPRESSED) {
                    if (!Func(false, true, PFile->m_RealName))
                        return;
                }
                else {
                    if (!Func(false, false, PFile->m_RealName))
                        return;
                }
            }
        }
    }
    if (!m_RealName.empty()) {
        Func(true, false, "..");
    }
}

CHsFolder *CHsFolder::GetFolderEx(const std::string& path) {
    if (path.empty())
        return this;

    std::string beg, rem;
    std::tie(beg, rem) = split_path(path, "/\\");

    SFileRec *PFile = GetFileRec(beg);
    if (PFile == NULL)
        return NULL;
    if (PFile->m_Type == FILEEC_FOLDER) {
        CHsFolder *PFolder = Child(PFile);
        if (!PFolder)
            return NULL;
        if (rem.empty())
            return PFolder;
        return PFolder->GetFolderEx(rem);
    }
    return NULL;
}

int CHsFolder::FindNext(SSearchRec &S) {
    SFileRec *PFile;

    do {
        PFile = GetFileRec(S.Ind);
        if (PFile == NULL) {
            return 1;
        }
        S.Name = PFile->m_RealName;
        S.T = PFile->m_Type;
        ++S.Ind;
    }
    while (PFile->m_Free != 0);
    return 0;
}

// **********************************************************************

CPackFile::CPackFile(CHeap *heap, const wchar *name) : m_Heap(heap), m_FileName{name} {
    m_Handle = 0xFFFFFFFF;
#ifdef SUPPORT_IN_MEMORY_STRUCTURES
    RESETFLAG(m_Flags, PFFLAG_EMPTY);
#endif

    m_RootFolder = NULL;
    m_RootOffset = 0;
    // m_ID = 0xFFFFFFFF;
    for (int i = 0; i < MAX_VIRTUAL_HANDLE_COUNT; ++i) {
        m_Handles[i].m_Free = true;
    }
}

CPackFile::~CPackFile() {
    Clear();
    ClosePacketFileEx();
}

void CPackFile::Clear(void) {
    for (int i = 0; i < MAX_VIRTUAL_HANDLE_COUNT; ++i) {
        if (!m_Handles[i].m_Free) {
            Close(i);
            m_Handles[i].m_Free = true;
        }
    }
}

bool CPackFile::OpenPacketFile(void) {
    if (m_Handle != 0xFFFFFFFF || m_RootFolder != NULL)
        ClosePacketFile();

#ifdef SUPPORT_IN_MEMORY_STRUCTURES
    if (FLAG(m_Flags, PFFLAG_EMPTY)) {
        m_RootOffset = 0;
        m_RootFolder = HNew(m_Heap) CHsFolder({});
        m_RootFolder->AllocEmptyFolder();
        return true;
    }
#endif

    // Открываем файл для записи/чтения
    if (IS_UNICODE()) {
        m_Handle = (uintptr_t)CreateFileW(m_FileName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    }
    else {
        m_Handle = (uintptr_t)CreateFileA(utils::from_wstring(m_FileName.c_str()).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    }

    if ((HANDLE)m_Handle == INVALID_HANDLE_VALUE) {
        std::fprintf(stderr, "pack open failed: %s\n", utils::from_wstring(m_FileName).c_str());
#ifdef RAISE_ALL_EXCEPTIONS
        ERROR_S((L"Error openning package file [READ]:" + m_FileName).c_str());
#endif
        m_Handle = 0xFFFFFFFF;
        return false;
    }
    std::fprintf(stderr, "pack open: %s\n", utils::from_wstring(m_FileName).c_str());
    // Теперь читаем данные о местоположении корневого каталога
    DWORD temp = 0;
    if (!::ReadFile((HANDLE)m_Handle, &m_RootOffset, 4, &temp, NULL)) {
#ifdef RAISE_ALL_EXCEPTIONS
        ERROR_S((L"Error openning package file [READ]:" + m_FileName).c_str());
#endif
        m_Handle = 0xFFFFFFFF;
        return false;
    }
    // Теперь читаем данные о корневой папке
    m_RootFolder = HNew(m_Heap) CHsFolder({}, m_Heap);
    if (!m_RootFolder->ReadFolder(m_Handle, m_RootOffset)) {
        std::fprintf(stderr, "pack directory read failed: %s root=%u\n", utils::from_wstring(m_FileName).c_str(), static_cast<unsigned>(m_RootOffset));
        HDelete(CHsFolder, m_RootFolder, m_Heap);
        m_RootFolder = NULL;
        ClosePacketFile();
#ifdef RAISE_ALL_EXCEPTIONS
        ERROR_S((L"Error reading file system of the package file: " + m_FileName).c_str());
#endif
        return false;
    }
    std::fprintf(stderr, "pack directory ok: %s\n", utils::from_wstring(m_FileName).c_str());
    return true;
}

bool CPackFile::ClosePacketFile(void) {
    if (m_Handle == 0xFFFFFFFF && m_RootFolder == NULL)
        return false;
    Clear();
    if (m_RootFolder) {
        HDelete(CHsFolder, m_RootFolder, m_Heap);
        m_RootFolder = NULL;
    }
    bool res;
    if (m_Handle != 0xFFFFFFFF) {
        res = CloseHandle((HANDLE)m_Handle) != FALSE;
    }
    else {
        res = true;
    }
    m_Handle = 0xFFFFFFFF;
    return res;
}

// bool    CPackFile::CreatePacketFileEx(void)

bool CPackFile::OpenPacketFileEx() {
    if (m_Handle != 0xFFFFFFFF || m_RootFolder != NULL)
        ClosePacketFileEx();

#ifdef SUPPORT_IN_MEMORY_STRUCTURES
    if (FLAG(m_Flags, PFFLAG_EMPTY)) {
        m_RootOffset = 0;
        m_RootFolder = HNew(m_Heap) CHsFolder({});
        m_RootFolder->AllocEmptyFolder();
        return true;
    }
#endif

    // Открываем файл для записи/чтения
    if (IS_UNICODE()) {
        m_Handle =
                (uintptr_t)CreateFileW(m_FileName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    }
    else {
        m_Handle =
                (uintptr_t)CreateFileA(utils::from_wstring(m_FileName.c_str()).c_str(), GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    }

    if ((HANDLE)m_Handle == INVALID_HANDLE_VALUE) {
#ifdef RAISE_ALL_EXCEPTIONS
        ERROR_S((L"Error openning package file [READ|WRITE]:" + m_FileName).c_str());
#endif
        m_Handle = 0xFFFFFFFF;
        return false;
    }
    // Теперь читаем данные о местоположении корневого каталога
    DWORD temp = 0;
    if (!::ReadFile((HANDLE)m_Handle, &m_RootOffset, 4, &temp, NULL)) {
#ifdef RAISE_ALL_EXCEPTIONS
        ERROR_S((L"Error openning package file [READ]:" + m_FileName).c_str());
#endif
        m_Handle = 0xFFFFFFFF;
        return false;
    }
    // Теперь читаем данные о корневой папке
    m_RootFolder = HNew(m_Heap) CHsFolder({}, m_Heap);
    if (!m_RootFolder->ReadFolder(m_Handle, m_RootOffset)) {
        HDelete(CHsFolder, m_RootFolder, m_Heap);
        m_RootFolder = NULL;
        ClosePacketFile();
#ifdef RAISE_ALL_EXCEPTIONS
        ERROR_S((L"Error reading file system of the package file: " + m_FileName).c_str());
#endif
        return false;
    }
    return true;
}

int CPackFile::GetFreeHandle(void) {
    for (int i = 0; i < MAX_VIRTUAL_HANDLE_COUNT; ++i) {
        if (m_Handles[i].m_Free) {
            return i;
        }
    }
    return -1;
}

DWORD CPackFile::Open(
    const std::string& filename,
    [[maybe_unused]] DWORD modeopen) {
    SFileRec *PFile;

    int H = GetFreeHandle();
    if (H < 0)
        return 0xFFFFFFFF;
    if (m_RootFolder == NULL) {
        ERROR_S(utils::format(L"Package not opened: %ls", utils::to_wstring(filename).c_str()));
    }
    PFile = m_RootFolder->GetFileRecEx(filename);
    if (PFile == NULL && m_RootFolder) {
        std::string leaf = filename;
        auto slash = leaf.find_last_of("/\\");
        if (slash != std::string::npos) leaf = leaf.substr(slash + 1);
        std::string found;
        if (m_RootFolder->FindLeaf(leaf, found)) {
            std::fprintf(stderr, "pack remap %s -> %s\n", filename.c_str(), found.c_str());
            PFile = m_RootFolder->GetFileRecEx(found);
        }
    }

    if (PFile == NULL) {
        std::fprintf(stderr, "pack miss %s\n", filename.c_str());
#ifdef HANDLE_OUT_OF_PACK_FILES
        // *** Если файл не найден, то пытаемся найти его на диске *** //
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(filename.c_str(), &fd);
        if (h = INVALID_HANDLE_VALUE)
            return 0xFFFFFFFF;
        FindClose(h);

        m_Handles[H].m_Handle = (DWORD)CreateFileA(filename.c_str(), modeopen, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        if ((HANDLE)m_Handles[H].m_Handle == INVALID_HANDLE_VALUE)
            return 0xFFFFFFFF;
        m_Handles[H].m_StartOffset = 0;
        m_Handles[H].m_Offset = 0;
        m_Handles[H].m_Size = ::GetFileSize((HANDLE)m_Handles[H].m_Handle, NULL);
        m_Handles[H].m_SouBuf = NULL;
        m_Handles[H].m_DesBuf = NULL;
        m_Handles[H].m_Compressed = false;
        m_Handles[H].m_Blocknumber = -1;
        if (m_Handles[H].m_Size == INVALID_FILE_SIZE) {
            ERROR_S(utils::format(L"File system error: %ls", utils::to_wstring(filename).c_str()));
        }

        DWORD Error = SetFilePointer((HANDLE)m_Handles[H].m_Handle, m_Handles[H].m_Offset, NULL, FILE_BEGIN);
        if (Error == 0xFFFFFFFF) {
            ERROR_S(utils::format(L"File system error: %ls", utils::to_wstring(filename).c_str()));
        }

        m_Handles[H].m_Free = false;
        return H;
#else
        return 0xFFFFFFFF;
#endif
    }

    // *** Файл был найден внутри пакетного файла - открываем его для чтения ***
    if (m_Handle == 0xFFFFFFFF)
        return 0xFFFFFFFF;

    m_Handles[H].m_Handle = m_Handle;
    m_Handles[H].m_StartOffset = PFile->m_Offset + 4;
    m_Handles[H].m_Offset = PFile->m_Offset + 4;
    m_Handles[H].m_Size = PFile->m_RealSize;
    m_Handles[H].m_Free = false;
    m_Handles[H].m_Compressed = PFile->m_Type == FILEEC_COMPRESSED;
    m_Handles[H].m_Blocknumber = -1;
    if (m_Handles[H].m_Compressed) {
        m_Handles[H].m_SouBuf = (BYTE *)HAlloc(72112, m_Heap);
        m_Handles[H].m_DesBuf = (BYTE *)HAlloc(65536, m_Heap);
    }
    else {
        m_Handles[H].m_SouBuf = NULL;
        m_Handles[H].m_DesBuf = NULL;
    };
    DWORD Error = SetFilePointer((HANDLE)m_Handles[H].m_Handle, m_Handles[H].m_Offset, NULL, FILE_BEGIN);
    if (Error == 0xFFFFFFFF) {
        ERROR_S(utils::format(L"Packet file system error: %ls", utils::to_wstring(filename).c_str()));
    }
    return H;
}

bool CPackFile::Close(DWORD Handle) {
    if (Handle == 0xFFFFFFFF)
        return false;
    if (Handle >= MAX_VIRTUAL_HANDLE_COUNT) {
        ERROR_S(L"Too big file handle number");
    }
    if (m_Handles[Handle].m_Free)
        return false;

    if (m_Handles[Handle].m_Handle == m_Handle) {
        // in pack file
        if (m_Handles[Handle].m_Compressed) {
            HFree(m_Handles[Handle].m_SouBuf, m_Heap);
            m_Handles[Handle].m_SouBuf = NULL;
            HFree(m_Handles[Handle].m_DesBuf, m_Heap);
            m_Handles[Handle].m_DesBuf = NULL;
        }
        m_Handles[Handle].m_Free = true;
        return true;
    }
#ifdef HANDLE_OUT_OF_PACK_FILES

    if (0 == ::CloseHandle((HANDLE)m_Handles[Handle].m_Handle)) {
        ERROR_S(L"Error closing file");
    }
    if (m_Handles[Handle].m_Compressed) {
        HFree(m_Handles[Handle].m_SouBuf, m_Heap);
        m_Handles[Handle].m_SouBuf = NULL;
        HFree(m_Handles[Handle].m_DesBuf, m_Heap);
        m_Handles[Handle].m_DesBuf = NULL;
    }
    m_Handles[Handle].m_Free = true;
#else
    return false;
#endif
}

DWORD CPackFile::SetCompressedBlockPointer(DWORD StartOffset, int nBlock) {
    DWORD Temp, Size;

    DWORD Offset = StartOffset;
    for (;;) {
        ::SetFilePointer((HANDLE)m_Handle, Offset, NULL, FILE_BEGIN);
        ::ReadFile((HANDLE)m_Handle, &Size, 4, &Temp, NULL);
        if (nBlock) {
            --nBlock;
            Offset += Size + 4;
        }
        else
            break;
    }
    return Size;
}

bool CPackFile::Read(DWORD Handle, void *buf, int Size) {
    DWORD Temp;
    int Block;
    DWORD IntBlockOffset;
    DWORD IntBlockSize;
    // #ifdef SUPPORT_COMPRESSION_OFF
    // #else
    DWORD CompSize;
    // #endif

    DWORD IntOffset;

    if (Handle == 0xFFFFFFFF)
        return false;
    if (Handle >= MAX_VIRTUAL_HANDLE_COUNT) {
        ERROR_S(L"Too big file handle number");
    }

    if (m_Handles[Handle].m_Free)
        return false;

    // *** Установка указателя на нужную позицию
    if (m_Handles[Handle].m_Compressed) {
        //  Определение номера блока, с которого начинаются читаемые данные
        BYTE *SouPtr = m_Handles[Handle].m_DesBuf;
        BYTE *DesPtr = (BYTE *)buf;
        while (Size) {
            // Определяем номер блока, чтение которого надо произвести с диска

            IntOffset = m_Handles[Handle].m_Offset - m_Handles[Handle].m_StartOffset;
            Block = IntOffset >> 16;  // 65536
            IntBlockOffset = IntOffset - (DWORD(Block) << 16);
            IntBlockSize = Size;
            if (IntBlockSize > (65536 - IntBlockOffset)) {
                IntBlockSize = 65536 - IntBlockOffset;
            }

            // Если блок не был расжат заранее, то расжимаем его
            if (m_Handles[Handle].m_Blocknumber != Block) {
                CompSize = SetCompressedBlockPointer(m_Handles[Handle].m_StartOffset, Block);
                BOOL res = ::ReadFile((HANDLE)m_Handle, m_Handles[Handle].m_SouBuf, CompSize, &Temp, NULL);
                if (res == FALSE)
                    return false;

                OKGF_ZLib_UnCompress2(m_Handles[Handle].m_DesBuf, 65536, m_Handles[Handle].m_SouBuf, CompSize);

                m_Handles[Handle].m_Blocknumber = Block;
            }
            // Теперь нам необходимо прочитать часть данных в буфер, указанный пользователем
            memcpy(DesPtr, SouPtr + IntBlockOffset, IntBlockSize);
            DesPtr += IntBlockSize;
            Size -= IntBlockSize;
            m_Handles[Handle].m_Offset = m_Handles[Handle].m_Offset + IntBlockSize;
        }
        return true;
    }
    else {
        ::SetFilePointer((HANDLE)m_Handles[Handle].m_Handle, m_Handles[Handle].m_Offset, NULL, FILE_BEGIN);
        BOOL res = ::ReadFile((HANDLE)m_Handles[Handle].m_Handle, buf, Size, &Temp, NULL);
        bool ret = (res != FALSE) && (Size == Temp);
        m_Handles[Handle].m_Offset = m_Handles[Handle].m_Offset + Temp;
        return ret;
    }
}

#ifdef HANDLE_OUT_OF_PACK_FILES
bool CPackFile::Write(DWORD Handle, const void *buf, int Size) {
    DWORD Temp;
    DWORD LocalOffset;

    if (Handle == 0xFFFFFFFF)
        return false;
    if (Handle >= MAX_VIRTUAL_HANDLE_COUNT) {
        ERROR_S(L"Too big file handle number");
    }
    if (m_Handles[Handle].m_Free)
        return false;

    // *** Установка указателя на нужную позицию
    if (m_Handles[Handle].m_Compressed) {
        ERROR_S(L"Error writing compressed file");
    }
    ::SetFilePointer((HANDLE)m_Handles[Handle].m_Handle, m_Handles[Handle].m_Offset, NULL, FILE_BEGIN);

    BOOL res = ::WriteFile((HANDLE)m_Handles[Handle].m_Handle, buf, Size, &Temp, NULL);
    bool ret = (res != FALSE) && (Size == Temp);
    m_Handles[Handle].m_Offset = m_Handles[Handle].m_Offset + Temp;
    LocalOffset = m_Handles[Handle].m_Offset - m_Handles[Handle].m_StartOffset;
    if (m_Handles[Handle].m_Size < LocalOffset) {
        m_Handles[Handle].m_Size = LocalOffset;
    }
}
#endif

bool CPackFile::SetPos(DWORD Handle, DWORD Pos, int ftype) {
    DWORD Error;
    int Block;

    if (Handle == 0xFFFFFFFF)
        return false;
    if (Handle >= MAX_VIRTUAL_HANDLE_COUNT) {
        ERROR_S(L"Too big file handle number");
    }
    if (m_Handles[Handle].m_Free)
        return false;

    // Пересчет позиции
    if (ftype == FILE_CURRENT) {
        Pos += m_Handles[Handle].m_Offset - m_Handles[Handle].m_StartOffset;
    }
    else if (ftype == FILE_END) {
        Pos = m_Handles[Handle].m_Size - Pos;
    }
    // {$ifdef SUPPORT_COMPRESSION_OFF}
    if (m_Handles[Handle].m_Compressed) {
        if (Pos > m_Handles[Handle].m_Size)
            return false;
        Block = Pos >> 16;
        if (Block != m_Handles[Handle].m_Blocknumber) {
            m_Handles[Handle].m_Blocknumber = -1;
        }
        m_Handles[Handle].m_Offset = Pos + m_Handles[Handle].m_StartOffset;
    }
    else {
        Error = SetFilePointer((HANDLE)m_Handles[Handle].m_Handle, m_Handles[Handle].m_StartOffset + Pos, NULL,
                               FILE_BEGIN);
        if (Error == 0xFFFFFFFF) {
            ERROR_S(L"Unable to set file pointer.");
            // then raise Exception.Create('Ошибка установки указателя в пакетном файле :'+m_filename);
        }
        m_Handles[Handle].m_Offset = Error;
    }
    return true;
}

DWORD CPackFile::GetPos(DWORD Handle) {
    if (Handle == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    if (Handle >= MAX_VIRTUAL_HANDLE_COUNT) {
        ERROR_S(L"Too big file handle number");
    }
    if (m_Handles[Handle].m_Free)
        return 0xFFFFFFFF;

    return m_Handles[Handle].m_Offset - m_Handles[Handle].m_StartOffset;
}

DWORD CPackFile::GetSize(DWORD Handle) {
    if (Handle == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    if (Handle >= MAX_VIRTUAL_HANDLE_COUNT) {
        ERROR_S(L"Too big file handle number");
    }
    if (m_Handles[Handle].m_Free)
        return 0xFFFFFFFF;
    return m_Handles[Handle].m_Size;
}

DWORD CPackFile::GetHandle(DWORD Handle) {
    if (Handle == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    if (Handle >= MAX_VIRTUAL_HANDLE_COUNT) {
        ERROR_S(L"Too big file handle number");
    }
    if (m_Handles[Handle].m_Free)
        return 0xFFFFFFFF;
    return m_Handles[Handle].m_Handle;
}

#ifdef HANDLE_OUT_OF_PACK_FILES
bool CPackFile::PathExists(const std::string& path) {
    if (m_RootFolder->PathExists(path))
        return true;

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(path.c_str(), &fd);
    if (h = INVALID_HANDLE_VALUE)
        return false;
    FindClose(h);
    return (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool CPackFile::FileExists(const std::string& path) {
    if (m_RootFolder->FileExists(path))
        return true;

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(path.c_str(), &fd);
    if (h = INVALID_HANDLE_VALUE)
        return false;
    FindClose(h);
    return true;
}
#endif

// Function    CPackFile.AddFiles(block:TBlockParEC;Var log:Text):boolean;

int CPackFile::FindFirst(
    const std::string& path,
    [[maybe_unused]] DWORD Attr,
    SSearchRec &S)
{
    S.Path = path;
    S.Ind = 0;
    S.Name = "";
    S.Folder = NULL;
    if (m_RootFolder == NULL)
        return 0;
    if (path.empty())
        S.Folder = m_RootFolder;
    else
        S.Folder = m_RootFolder->GetFolderEx(S.Path);

    return S.Folder->FindNext(S);
}

// **********************************************************************

void CPackCollection::Clear(void)
{
    for (auto item : m_PackFiles)
    {
        HDelete(CPackFile, item, nullptr);
    }
    m_PackFiles.clear();
}

void CPackCollection::AddPacketFile(const wchar *name)
{
    auto f = HNew(m_Heap) CPackFile(m_Heap, name);
    m_PackFiles.push_back(f);
}

void CPackCollection::DelPacketFile(const wchar *name)
{
    auto pred = [name](const CPackFile* item){ return item->GetName() == name; };
    auto item = std::find_if(m_PackFiles.begin(), m_PackFiles.end(), pred);
    if (item != m_PackFiles.end())
    {
        m_PackFiles.erase(item);
    }
}

bool CPackCollection::OpenPacketFiles(void)
{
    int opened = 0;
    for (auto* pack : m_PackFiles)
    {
        if (pack->OpenPacketFile()) {
            ++opened;
        }
    }
    return opened > 0;
}

bool CPackCollection::ClosePacketFiles(void)
{
    for (auto item : m_PackFiles)
    {
        item->ClosePacketFile();
    }

    return true;
}

bool CPackCollection::OpenPacketFilesEx(void)
{
    int iter = 0;
    for (; iter < m_PackFiles.size(); ++iter)
    {
        if (!m_PackFiles[iter]->OpenPacketFileEx())
        {
            break;
        }
    }

    // if not all the files were opened, we'll close the open ones
    if (iter != m_PackFiles.size())
    {
        for (; iter >= 0; --iter)
        {
            m_PackFiles[iter]->ClosePacketFileEx();
        }

        return false;
    }

    return true;
}

bool CPackCollection::ClosePacketFilesEx(void)
{
    for (auto item : m_PackFiles)
    {
        item->ClosePacketFileEx();
    }

    return true;
}

// ******** Процедуры для работы файлами ***********//

bool CPackCollection::FileExists(const std::string& name)
{
    for (auto item : m_PackFiles)
    {
        if(item->FileExists(name))
        {
            return true;
        }
    }

    return false;
}

bool CPackCollection::PathExists(const std::string& path)
{
    for (auto item : m_PackFiles)
    {
        if(item->PathExists(path))
        {
            return true;
        }
    }

    return false;
}

// ******* работа с виртуальными номерами объектов CPackFile
DWORD CPackCollection::Open(const std::string& name, DWORD modeopen) {
    DWORD Handle;
    DWORD Counter = 0;

    for (auto item : m_PackFiles)
    {
        Handle = item->Open(name, modeopen);
        if (Handle != 0xFFFFFFFF)
            break;
        ++Counter;
    }

    if (Handle == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    return Handle + (Counter << MAX_VIRTUAL_HANDLE_COUNT_BITS);
}

bool CPackCollection::Close(DWORD Handle) {
    ASSERT(MAX_VIRTUAL_HANDLE_COUNT == (1 << MAX_VIRTUAL_HANDLE_COUNT_BITS));
    int number = Handle >> MAX_VIRTUAL_HANDLE_COUNT_BITS;
    CPackFile* P = GetPacketFile(number);
    if (P == NULL)
        return false;
    return P->Close(Handle & (MAX_VIRTUAL_HANDLE_COUNT - 1));
}

bool CPackCollection::Read(DWORD Handle, void *Buf, int Size) {
    ASSERT(MAX_VIRTUAL_HANDLE_COUNT == (1 << MAX_VIRTUAL_HANDLE_COUNT_BITS));

    int number = Handle >> MAX_VIRTUAL_HANDLE_COUNT_BITS;
    CPackFile* P = GetPacketFile(number);
    if (P == NULL)
        return false;
    return P->Read(Handle & (MAX_VIRTUAL_HANDLE_COUNT - 1), Buf, Size);
}

// bool  CPackCollection::Write(DWORD Handle, const void *Buf, int Size)

bool CPackCollection::SetPos(DWORD Handle, DWORD Pos, int ftype) {
    ASSERT(MAX_VIRTUAL_HANDLE_COUNT == (1 << MAX_VIRTUAL_HANDLE_COUNT_BITS));

    int number = Handle >> MAX_VIRTUAL_HANDLE_COUNT_BITS;
    CPackFile* P = GetPacketFile(number);
    if (P == NULL)
        return false;
    return P->SetPos(Handle & (MAX_VIRTUAL_HANDLE_COUNT - 1), Pos, ftype);
}

DWORD CPackCollection::GetPos(DWORD Handle) {
    ASSERT(MAX_VIRTUAL_HANDLE_COUNT == (1 << MAX_VIRTUAL_HANDLE_COUNT_BITS));

    int number = Handle >> MAX_VIRTUAL_HANDLE_COUNT_BITS;
    CPackFile* P = GetPacketFile(number);
    if (P == NULL)
        return 0xFFFFFFFF;
    return P->GetPos(Handle & (MAX_VIRTUAL_HANDLE_COUNT - 1));
}

DWORD CPackCollection::GetSize(DWORD Handle) {
    ASSERT(MAX_VIRTUAL_HANDLE_COUNT == (1 << MAX_VIRTUAL_HANDLE_COUNT_BITS));

    int number = Handle >> MAX_VIRTUAL_HANDLE_COUNT_BITS;
    CPackFile* P = GetPacketFile(number);
    if (P == NULL)
        return 0xFFFFFFFF;
    return P->GetSize(Handle & (MAX_VIRTUAL_HANDLE_COUNT - 1));
}

DWORD CPackCollection::GetHandle(DWORD Handle) {
    ASSERT(MAX_VIRTUAL_HANDLE_COUNT == (1 << MAX_VIRTUAL_HANDLE_COUNT_BITS));

    int number = Handle >> MAX_VIRTUAL_HANDLE_COUNT_BITS;
    CPackFile* P = GetPacketFile(number);
    if (P == NULL)
        return 0xFFFFFFFF;
    return P->GetHandle(Handle & (MAX_VIRTUAL_HANDLE_COUNT - 1));
}

// bool CPackCollection::UnpackFile(const std::string& souname,const std::string& desname)

}  // namespace Base

// #endif
