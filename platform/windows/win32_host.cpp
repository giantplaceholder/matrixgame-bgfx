// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

// real Win32, do not include the fake windows.h

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "host_api.h"

#include <cstring>
#include <string>

namespace {

void fill_find(const WIN32_FIND_DATAW& data, MgFindData* out) {
    if (!out) return;
    std::memset(out, 0, sizeof(*out));
    out->attributes = data.dwFileAttributes;
    std::wstring wide(data.cFileName);
    std::string narrow;
    narrow.reserve(wide.size());
    for (wchar_t ch : wide) narrow.push_back(static_cast<char>(ch));
    std::strncpy(out->name, narrow.c_str(), sizeof(out->name) - 1);
}

}  // namespace

extern "C" void* mgwin_create_file(const wchar_t* name, uint32_t disposition) {
    if (!name || !name[0]) return INVALID_HANDLE_VALUE;
    DWORD access = GENERIC_READ;
    DWORD disp = OPEN_EXISTING;
    if (disposition == CREATE_ALWAYS) {
        access = GENERIC_READ | GENERIC_WRITE;
        disp = CREATE_ALWAYS;
    }
    return CreateFileW(name, access, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, disp, FILE_ATTRIBUTE_NORMAL, nullptr);
}

extern "C" int mgwin_read(void* file, void* buffer, uint32_t bytes, uint32_t* read_bytes) {
    DWORD got = 0;
    if (!ReadFile(file, buffer, bytes, &got, nullptr)) return 0;
    if (read_bytes) *read_bytes = got;
    return 1;
}

extern "C" int mgwin_write(void* file, const void* buffer, uint32_t bytes, uint32_t* written) {
    DWORD got = 0;
    if (!WriteFile(file, buffer, bytes, &got, nullptr)) return 0;
    if (written) *written = got;
    return 1;
}

extern "C" uint32_t mgwin_size(void* file, uint32_t* high) {
    DWORD hi = 0;
    DWORD lo = GetFileSize(file, &hi);
    if (high) *high = hi;
    return lo;
}

extern "C" uint32_t mgwin_seek(void* file, int32_t distance, uint32_t method) {
    DWORD whence = FILE_BEGIN;
    if (method == FILE_CURRENT) whence = FILE_CURRENT;
    else if (method == FILE_END) whence = FILE_END;
    return SetFilePointer(file, distance, nullptr, whence);
}

extern "C" int mgwin_close(void* file) {
    return CloseHandle(file) ? 1 : 0;
}

extern "C" void* mgwin_find_first(const wchar_t* pattern, MgFindData* out) {
    if (!pattern) return INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW data{};
    HANDLE found = FindFirstFileW(pattern, &data);
    if (found == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;
    fill_find(data, out);
    return found;
}

extern "C" int mgwin_find_next(void* find, MgFindData* out) {
    WIN32_FIND_DATAW data{};
    if (!FindNextFileW(find, &data)) return 0;
    fill_find(data, out);
    return 1;
}

extern "C" int mgwin_find_close(void* find) {
    return FindClose(find) ? 1 : 0;
}

extern "C" int mgwin_qpc(int64_t* out) {
    LARGE_INTEGER value{};
    if (!QueryPerformanceCounter(&value)) return 0;
    if (out) *out = value.QuadPart;
    return 1;
}

extern "C" int mgwin_qpf(int64_t* out) {
    LARGE_INTEGER value{};
    if (!QueryPerformanceFrequency(&value)) return 0;
    if (out) *out = value.QuadPart;
    return 1;
}
