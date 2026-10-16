// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "windows.h"

#include "host_api.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
std::wstring g_command_line = L"matrixgame";
std::vector<std::wstring> g_args;
std::vector<wchar_t*> g_arg_ptrs;
}

std::string mg_narrow(const wchar_t* text) {
    std::string out;
    if (!text) return out;
    for (; *text; ++text) out.push_back(static_cast<char>(*text == L'\\' ? L'/' : *text));
    return out;
}

std::wstring mg_widen(const char* text) {
    std::wstring out;
    if (!text) return out;
    for (; *text; ++text) out.push_back(static_cast<wchar_t>(*text));
    return out;
}

HANDLE CreateFileA(const char* name, DWORD, DWORD, void*, DWORD disposition, DWORD, HANDLE) {
    if (!name) return INVALID_HANDLE_VALUE;
    std::wstring wide = mg_widen(name);
    for (wchar_t& ch : wide) if (ch == L'/') ch = L'\\';
    return mgwin_create_file(wide.c_str(), disposition);
}

HANDLE CreateFileW(const wchar_t* name, DWORD access, DWORD share, void* security, DWORD disposition, DWORD flags, HANDLE template_file) {
    return CreateFileA(mg_narrow(name).c_str(), access, share, security, disposition, flags, template_file);
}

BOOL ReadFile(HANDLE file, void* buffer, DWORD bytes, DWORD* read, void*) {
    return mgwin_read(file, buffer, bytes, read) ? TRUE : FALSE;
}

BOOL WriteFile(HANDLE file, const void* buffer, DWORD bytes, DWORD* written, void*) {
    return mgwin_write(file, buffer, bytes, written) ? TRUE : FALSE;
}

DWORD GetFileSize(HANDLE file, DWORD* high) {
    return mgwin_size(file, high);
}

DWORD SetFilePointer(HANDLE file, LONG distance, LONG* high, DWORD method) {
    DWORD pos = mgwin_seek(file, distance, method);
    if (high) *high = 0;
    return pos;
}

BOOL CloseHandle(HANDLE handle) {
    return mgwin_close(handle) ? TRUE : FALSE;
}

HANDLE FindFirstFileA(const char* name, WIN32_FIND_DATAA* data) {
    if (!name || !data) return INVALID_HANDLE_VALUE;
    std::wstring wide = mg_widen(name);
    for (wchar_t& ch : wide) if (ch == L'/') ch = L'\\';
    MgFindData found{};
    void* handle = mgwin_find_first(wide.c_str(), &found);
    if (handle == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;
    std::memset(data, 0, sizeof(*data));
    data->dwFileAttributes = found.attributes;
    std::strncpy(data->cFileName, found.name, MAX_PATH - 1);
    return handle;
}

HANDLE FindFirstFileW(const wchar_t* name, WIN32_FIND_DATAW* data) {
    WIN32_FIND_DATAA narrow_data{};
    HANDLE handle = FindFirstFileA(mg_narrow(name).c_str(), &narrow_data);
    if (handle == INVALID_HANDLE_VALUE || !data) return handle;
    std::memset(data, 0, sizeof(*data));
    data->dwFileAttributes = narrow_data.dwFileAttributes;
    std::wstring wide = mg_widen(narrow_data.cFileName);
    std::wcsncpy(data->cFileName, wide.c_str(), MAX_PATH - 1);
    return handle;
}

BOOL FindNextFileA(HANDLE find, WIN32_FIND_DATAA* data) {
    if (!data) return FALSE;
    MgFindData found{};
    if (!mgwin_find_next(find, &found)) return FALSE;
    std::memset(data, 0, sizeof(*data));
    data->dwFileAttributes = found.attributes;
    std::strncpy(data->cFileName, found.name, MAX_PATH - 1);
    return TRUE;
}

BOOL FindNextFileW(HANDLE find, WIN32_FIND_DATAW* data) {
    WIN32_FIND_DATAA narrow_data{};
    if (!FindNextFileA(find, &narrow_data) || !data) return FALSE;
    std::memset(data, 0, sizeof(*data));
    data->dwFileAttributes = narrow_data.dwFileAttributes;
    std::wstring wide = mg_widen(narrow_data.cFileName);
    std::wcsncpy(data->cFileName, wide.c_str(), MAX_PATH - 1);
    return TRUE;
}

BOOL FindClose(HANDLE find) {
    return mgwin_find_close(find) ? TRUE : FALSE;
}

BOOL QueryPerformanceCounter(LARGE_INTEGER* counter) {
    int64_t value = 0;
    if (!mgwin_qpc(&value)) return FALSE;
    if (counter) counter->QuadPart = value;
    return TRUE;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER* frequency) {
    int64_t value = 0;
    if (!mgwin_qpf(&value)) return FALSE;
    if (frequency) frequency->QuadPart = value;
    return TRUE;
}

intptr_t g_wndproc = 1;

ATOM RegisterClassExW(const WNDCLASSEXW* wc) {
    if (wc && wc->lpfnWndProc) g_wndproc = reinterpret_cast<intptr_t>(wc->lpfnWndProc);
    return 1;
}

HWND CreateWindowExA(DWORD, const char*, const char* title, DWORD, int, int, int w, int h, HWND, void*, HINSTANCE, void*) {
    mg_sdl_open(title, w, h);
    return reinterpret_cast<HWND>(static_cast<uintptr_t>(1));
}

BOOL AdjustWindowRectEx(RECT*, DWORD, BOOL, DWORD) { return TRUE; }

// close the SDL window and the device
BOOL DestroyWindow(HWND) {
    mg_sdl_close();
    return TRUE;
}

BOOL GetClientRect(HWND, RECT* rect) {
    int w = 1280, h = 720;
    mg_sdl_size(&w, &h);
    if (rect) { rect->left = 0; rect->top = 0; rect->right = w; rect->bottom = h; }
    return TRUE;
}

intptr_t GetWindowLongPtr(HWND, int) { return g_wndproc; }
intptr_t SetWindowLongPtrW(HWND, int, intptr_t value) { intptr_t old = g_wndproc; g_wndproc = value ? value : 1; return old; }

BOOL PeekMessageW(MSG* msg, HWND, UINT, UINT, UINT) {
    uint32_t message = 0;
    uintptr_t wparam = 0;
    intptr_t lparam = 0;
    if (!mg_sdl_poll(&message, &wparam, &lparam) || !msg) return FALSE;
    std::memset(msg, 0, sizeof(*msg));
    msg->message = message;
    msg->wParam = wparam;
    msg->lParam = lparam;
    msg->hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(1));
    return TRUE;
}

LRESULT DispatchMessageW(const MSG* msg) {
    if (!msg) return 0;
    if (g_wndproc > 1) {
        auto proc = reinterpret_cast<WNDPROC>(g_wndproc);
        return proc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
    }
    return 0;
}

BOOL ShowWindow(HWND, int) { return TRUE; }
int MessageBoxA(HWND, const char* text, const char* title, UINT) {
    std::fprintf(stderr, "%s: %s\n", title ? title : "Message", text ? text : "");
    return 0;
}
BOOL ClipCursor(const RECT*) { return TRUE; }
void timeBeginPeriod(UINT) {}
void timeEndPeriod(UINT) {}
wchar_t* GetCommandLineW() { return g_command_line.data(); }
wchar_t** CommandLineToArgvW(const wchar_t*, int* count) {
    g_arg_ptrs.clear();
    for (auto& arg : g_args) g_arg_ptrs.push_back(arg.data());
    if (count) *count = static_cast<int>(g_arg_ptrs.size());
    return g_arg_ptrs.data();
}

void mg_set_command_line(int argc, char** argv) {
    g_args.clear();
    g_command_line.clear();
    for (int i = 0; i < argc; ++i) {
        std::wstring wide = mg_widen(argv[i]);
        if (i) g_command_line.push_back(L' ');
        g_command_line += wide;
        g_args.push_back(wide);
    }
}

