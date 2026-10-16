// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

// Enough of the Win32 file/heap surface for the MatrixGame pack loader on Linux
// wchar_t is 32-bit here. Paths used by the engine are ASCII

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <vector>

using BOOL = int;
using BYTE = unsigned char;
using WORD = unsigned short;
using DWORD = unsigned int;
using LONG = int;
using ULONG = unsigned int;
using UINT = unsigned int;
using HRESULT = long;
using HANDLE = void*;
using HINSTANCE = void*;
using HWND = void*;
using HMODULE = void*;
using LPVOID = void*;
using LPCSTR = const char*;
using LPSTR = char*;
using LPCWSTR = const wchar_t*;
using LPWSTR = wchar_t*;
using LPTSTR = wchar_t*;

constexpr BOOL TRUE = 1;
constexpr BOOL FALSE = 0;
// Only define when absent: GCC predefines these, and an unconditional empty
// define erases __declspec(dllexport) on MinGW
#if !defined(__cdecl)
#define __cdecl
#endif
#if !defined(__stdcall)
#define __stdcall
#endif
#if !defined(__declspec)
#define __declspec(x)
#endif
// MATRIXGAMEDLL_API belongs to MatrixGameDll.hpp; do not redefine it here
constexpr int VK_OEM_2 = 0xbf;
#define CALLBACK
#define APIENTRY
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

using LONG_PTR = intptr_t;
using WPARAM = uintptr_t;
using LPARAM = intptr_t;
using LRESULT = intptr_t;
using ATOM = unsigned short;
using WNDPROC = LRESULT (*)(HWND, UINT, WPARAM, LPARAM);
struct WNDCLASSEXW { uint32_t cbSize; uint32_t style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra; HINSTANCE hInstance; void* hIcon; void* hCursor; void* hbrBackground; const char* lpszMenuName; const char* lpszClassName; void* hIconSm; };
using WNDCLASSEX = WNDCLASSEXW;
constexpr uint32_t CS_HREDRAW = 2;
constexpr uint32_t CS_VREDRAW = 1;
constexpr uint32_t CS_DBLCLKS = 8;
struct MSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; int pt_x, pt_y; };
struct RECT { int left, top, right, bottom; };
constexpr UINT WM_QUIT = 0x12;
constexpr UINT WM_PAINT = 0x0f;
constexpr UINT WM_KEYDOWN = 0x100;
constexpr UINT WM_KEYUP = 0x101;
constexpr UINT WM_MOUSEMOVE = 0x200;
constexpr UINT WM_LBUTTONDOWN = 0x201;
constexpr UINT WM_LBUTTONUP = 0x202;
constexpr UINT WM_LBUTTONDBLCLK = 0x203;
constexpr UINT WM_RBUTTONDOWN = 0x204;
constexpr UINT WM_RBUTTONUP = 0x205;
constexpr UINT WM_RBUTTONDBLCLK = 0x206;
constexpr UINT WM_MBUTTONDOWN = 0x207;
constexpr UINT WM_MBUTTONUP = 0x208;
constexpr UINT WM_MOUSEWHEEL = 0x20a;
constexpr UINT WM_SYSKEYDOWN = 0x104;
constexpr UINT WM_SYSKEYUP = 0x105;
constexpr UINT WM_ACTIVATEAPP = 0x1c;
constexpr UINT WM_CLOSE = 0x10;
constexpr UINT PM_REMOVE = 1;
constexpr int SW_MAXIMIZE = 3;
constexpr int SW_RESTORE = 9;
inline LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
constexpr UINT WM_SETCURSOR = 0x20;
constexpr UINT WM_DESTROY = 0x02;
// the battle window has to close or it stays over the host menu
BOOL DestroyWindow(HWND);
inline BOOL UnregisterClass(const char*, HINSTANCE) { return TRUE; }
constexpr int MB_OK = 0;
constexpr int GWLP_WNDPROC = -4;
constexpr uint32_t WS_OVERLAPPED = 0;
constexpr uint32_t WS_BORDER = 0;
constexpr uint32_t WS_CAPTION = 0;
constexpr uint32_t WS_SYSMENU = 0;
constexpr uint32_t WS_POPUP = 0x80000000;
constexpr int VK_LBUTTON = 1;
constexpr int VK_RBUTTON = 2;
constexpr int VK_MBUTTON = 4;
constexpr int VK_ESCAPE = 0x1b;
constexpr int IDC_ARROW = 32512;
using HCURSOR = void*;
inline HCURSOR LoadCursor(HINSTANCE, int) { return nullptr; }
intptr_t GetWindowLongPtr(HWND, int);

ATOM RegisterClassExW(const WNDCLASSEXW*);
#define RegisterClassEx RegisterClassExW
HWND CreateWindowExA(DWORD, const char*, const char*, DWORD, int, int, int, int, HWND, void*, HINSTANCE, void*);
#define CreateWindowW(cls, title, style, x, y, w, h, parent, menu, inst, param) CreateWindowExA(0, cls, title, style, x, y, w, h, parent, menu, inst, param)
#define CreateWindow CreateWindowW
BOOL AdjustWindowRectEx(RECT*, DWORD, BOOL, DWORD);
BOOL GetClientRect(HWND, RECT*);
intptr_t SetWindowLongPtrW(HWND, int, intptr_t);
#define SetWindowLongPtr SetWindowLongPtrW
BOOL PeekMessageW(MSG*, HWND, UINT, UINT, UINT);
#define PeekMessage PeekMessageW
LRESULT DispatchMessageW(const MSG*);
#define DispatchMessage DispatchMessageW
BOOL ShowWindow(HWND, int);
int MessageBoxA(HWND, const char*, const char*, UINT);
#define MessageBox MessageBoxA
BOOL ClipCursor(const RECT*);
void timeBeginPeriod(UINT);
void timeEndPeriod(UINT);
wchar_t* GetCommandLineW();
wchar_t** CommandLineToArgvW(const wchar_t*, int*);
#define ZeroMemory(p, n) std::memset((p), 0, (n))
using HDC = void*;
using HKEY = void*;
constexpr HKEY HKEY_CURRENT_USER = nullptr;
constexpr HKEY HKEY_LOCAL_MACHINE = nullptr;
constexpr HKEY HKEY_CLASSES_ROOT = nullptr;
constexpr DWORD REG_DWORD = 4;
constexpr DWORD REG_MULTI_SZ = 7;
constexpr long ERROR_SUCCESS = 0;
constexpr DWORD KEY_READ = 0x20019;
constexpr DWORD KEY_WRITE = 0x20006;
constexpr DWORD REG_SZ = 1;
inline DWORD GetVersion() { return 0; }
inline long RegOpenKeyExW(HKEY, const wchar_t*, DWORD, DWORD, HKEY*) { return 1; }
inline long RegOpenKeyExA(HKEY, const char*, DWORD, DWORD, HKEY*) { return 1; }
inline long RegCreateKeyExW(HKEY, const wchar_t*, DWORD, wchar_t*, DWORD, DWORD, void*, HKEY*, DWORD*) { return 1; }
inline long RegCreateKeyExA(HKEY, const char*, DWORD, char*, DWORD, DWORD, void*, HKEY*, DWORD*) { return 1; }
inline long RegQueryValueExW(HKEY, const wchar_t*, DWORD*, DWORD*, BYTE*, DWORD*) { return 1; }
inline long RegQueryValueExA(HKEY, const char*, DWORD*, DWORD*, BYTE*, DWORD*) { return 1; }
inline long RegSetValueExW(HKEY, const wchar_t*, DWORD, DWORD, const BYTE*, DWORD) { return 1; }
inline long RegSetValueExA(HKEY, const char*, DWORD, DWORD, const BYTE*, DWORD) { return 1; }
inline long RegCloseKey(HKEY) { return 0; }
using HBITMAP = void*;
using HICON = void*;
using HCURSOR = void*;
inline BOOL SetCursor(HCURSOR) { return TRUE; }
constexpr int VK_SPACE = 0x20;
constexpr int VK_RETURN = 0x0d;
constexpr int VK_SHIFT = 0x10;
constexpr int VK_F1 = 0x70;
constexpr int VK_F2 = 0x71;
constexpr int VK_F3 = 0x72;
constexpr int VK_F4 = 0x73;
constexpr int VK_F5 = 0x74;
constexpr int VK_F6 = 0x75;
constexpr int VK_F7 = 0x76;
constexpr int VK_F8 = 0x77;
constexpr int VK_F10 = 0x79;
constexpr int VK_F11 = 0x7A;
constexpr int VK_F12 = 0x7B;
#define MoveMemory(d, s, n) std::memmove((d), (s), (n))
#define _alloca(n) __builtin_alloca(n)
constexpr int VK_OEM_PERIOD = 0xbe;
constexpr int VK_OEM_3 = 0xc0;
using INT32 = int32_t;
struct POINT { int x, y; };
using LPPOINT = POINT*;
using LPRECT = RECT*;
struct ICONINFO { int fIcon; DWORD xHotspot, yHotspot; HBITMAP hbmMask, hbmColor; };
struct ITEMIDLIST { int dummy; };
struct PROCESS_INFORMATION { void* hProcess; void* hThread; DWORD dwProcessId, dwThreadId; };
struct STARTUPINFOA { DWORD cb; };
constexpr uint32_t CSIDL_PERSONAL = 5;
constexpr uint32_t DLL_PROCESS_ATTACH = 1;
constexpr uint32_t DLL_PROCESS_DETACH = 0;
constexpr uint32_t DLL_THREAD_ATTACH = 2;
constexpr uint32_t DLL_THREAD_DETACH = 3;
constexpr uint32_t DT_LEFT = 0;
constexpr uint32_t DT_NOCLIP = 0x100;
constexpr uint32_t DT_SINGLELINE = 0x20;
constexpr uint32_t DT_VCENTER = 4;
constexpr uint32_t FW_NORMAL = 400;
constexpr uint32_t DEFAULT_CHARSET = 1;
constexpr uint32_t OUT_DEFAULT_PRECIS = 0;
constexpr uint32_t DEFAULT_PITCH = 0;
constexpr uint32_t PROOF_QUALITY = 2;
constexpr uint32_t SRCCOPY = 0xcc0020;
constexpr uint32_t WS_VISIBLE = 0x10000000;
constexpr uint32_t SWP_NOMOVE = 2;
constexpr uint32_t SWP_NOZORDER = 4;
constexpr uint32_t SWP_NOACTIVATE = 0x10;
constexpr uint32_t SWP_NOOWNERZORDER = 0x200;
constexpr long NOERROR = 0;
inline HDC GetDC(HWND) { return reinterpret_cast<HDC>(static_cast<uintptr_t>(1)); }
inline int ReleaseDC(HWND, HDC) { return 1; }
inline HDC CreateCompatibleDC(HDC) { return reinterpret_cast<HDC>(static_cast<uintptr_t>(1)); }
inline void* SelectObject(HDC, void* obj) { return obj ? obj : reinterpret_cast<void*>(static_cast<uintptr_t>(1)); }
inline HBITMAP CreateCompatibleBitmap(HDC, int, int) { return nullptr; }
inline BOOL BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD) { return TRUE; }
inline BOOL ClientToScreen(HWND, POINT*) { return TRUE; }
inline BOOL GetWindowRect(HWND, RECT*) { return TRUE; }
inline BOOL MoveWindow(HWND, int, int, int, int, BOOL) { return TRUE; }
inline BOOL SetWindowPos(HWND, HWND, int, int, int, int, UINT) { return TRUE; }
inline BOOL UpdateWindow(HWND) { return TRUE; }
// Win32 ShowCursor returns the show/hide *count*, negative once hidden. The
int mg_show_cursor(int show);
inline int ShowCursor(int show) { return mg_show_cursor(show); }
int mg_create_directory(const wchar_t* path);
inline BOOL CreateDirectoryW(const wchar_t* path, void*) { return mg_create_directory(path) ? TRUE : FALSE; }
inline BOOL DeleteFileW(const wchar_t*) { return TRUE; }
inline BOOL CreateProcess(const char*, char*, void*, void*, BOOL, DWORD, void*, const char*, STARTUPINFOA*, PROCESS_INFORMATION*) { return FALSE; }
inline HICON CreateIconIndirect(ICONINFO*) { return nullptr; }
inline BOOL DestroyIcon(HICON) { return TRUE; }
// FALSE is 0, and PathToOutputFiles treats 0 as NOERROR, then reads an
// uninitialized path. A non-zero failure sends screenshots to .\Screenshots
inline BOOL SHGetSpecialFolderLocation(HWND, int, ITEMIDLIST**) { return 1; }
inline BOOL SHGetPathFromIDListW(ITEMIDLIST*, wchar_t* path) { if (path) path[0] = 0; return FALSE; }
inline void* SetUnhandledExceptionFilter(void*) { return nullptr; }
using HRSRC = void*;
using HGLOBAL = void*;
// Out of line on purpose. Inline constants let the compiler delete the
HMODULE GetModuleHandle(const char*);
HRSRC FindResource(HMODULE, const wchar_t*, const wchar_t*);
HGLOBAL LoadResource(HMODULE, HRSRC);
void* LockResource(HGLOBAL);
DWORD SizeofResource(HMODULE, HRSRC);
#define MAKEINTRESOURCE(x) ((wchar_t*)(uintptr_t)(x))
#define RT_FONT ((wchar_t*)8)
HANDLE AddFontMemResourceEx(void*, DWORD, void*, DWORD*);
struct SIZE { int cx, cy; };
struct BITMAPFILEHEADER { uint16_t bfType; uint32_t bfSize; uint16_t bfReserved1, bfReserved2; uint32_t bfOffBits; } __attribute__((packed));
struct BITMAPINFOHEADER { uint32_t biSize; int32_t biWidth, biHeight; uint16_t biPlanes, biBitCount; uint32_t biCompression, biSizeImage; int32_t biXPelsPerMeter, biYPelsPerMeter; uint32_t biClrUsed, biClrImportant; };;
inline BOOL GetTextExtentPoint32W(HDC, const wchar_t*, int n, SIZE* size) { if (size) { size->cx = (n > 0 ? n : 1) * 4; size->cy = 13; } return TRUE; }
constexpr uint32_t DT_CALCRECT = 0x400;
constexpr uint32_t DT_CENTER = 1;
constexpr uint32_t DT_RIGHT = 2;
constexpr uint32_t DT_TOP = 0;
constexpr uint32_t DT_BOTTOM = 8;
constexpr uint32_t DT_WORDBREAK = 0x10;
struct EXCEPTION_POINTERS { void* ExceptionRecord; void* ContextRecord; };
using PEXCEPTION_POINTERS = EXCEPTION_POINTERS*;
constexpr long EXCEPTION_EXECUTE_HANDLER = 1;
inline LONG SetWindowLong(HWND, int, LONG) { return 0; }
inline BOOL UnionRect(RECT* out, const RECT*, const RECT*) { if (out) *out = {}; return TRUE; }
#define LOWORD(x) (static_cast<uint16_t>(static_cast<uintptr_t>(x) & 0xffff))
#define HIWORD(x) (static_cast<uint16_t>((static_cast<uintptr_t>(x) >> 16) & 0xffff))
void mg_set_command_line(int argc, char** argv);
constexpr DWORD GENERIC_READ = 0x80000000u;
constexpr DWORD GENERIC_WRITE = 0x40000000u;
constexpr DWORD FILE_SHARE_READ = 1;
constexpr DWORD FILE_SHARE_WRITE = 2;
constexpr DWORD OPEN_EXISTING = 3;
constexpr DWORD CREATE_ALWAYS = 2;
constexpr DWORD FILE_ATTRIBUTE_NORMAL = 0x80;
constexpr DWORD FILE_ATTRIBUTE_DIRECTORY = 0x10;
constexpr DWORD FILE_BEGIN = 0;
constexpr DWORD FILE_CURRENT = 1;
constexpr DWORD FILE_END = 2;
constexpr DWORD MAX_PATH = 260;

struct FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};

struct WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    char cFileName[MAX_PATH];
    char cAlternateFileName[14];
};

struct WIN32_FIND_DATAW {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    wchar_t cFileName[MAX_PATH];
    wchar_t cAlternateFileName[14];
};

HANDLE CreateFileA(const char* name, DWORD access, DWORD share, void* security, DWORD disposition, DWORD flags, HANDLE template_file);
HANDLE CreateFileW(const wchar_t* name, DWORD access, DWORD share, void* security, DWORD disposition, DWORD flags, HANDLE template_file);
BOOL ReadFile(HANDLE file, void* buffer, DWORD bytes, DWORD* read, void* overlapped);
BOOL WriteFile(HANDLE file, const void* buffer, DWORD bytes, DWORD* written, void* overlapped);
DWORD GetFileSize(HANDLE file, DWORD* high);
DWORD SetFilePointer(HANDLE file, LONG distance, LONG* high, DWORD method);
BOOL CloseHandle(HANDLE handle);
HANDLE FindFirstFileA(const char* name, WIN32_FIND_DATAA* data);
HANDLE FindFirstFileW(const wchar_t* name, WIN32_FIND_DATAW* data);
BOOL FindNextFileA(HANDLE find, WIN32_FIND_DATAA* data);
BOOL FindNextFileW(HANDLE find, WIN32_FIND_DATAW* data);
BOOL FindClose(HANDLE find);
BOOL QueryPerformanceCounter(union _LARGE_INTEGER* counter);
BOOL QueryPerformanceFrequency(union _LARGE_INTEGER* frequency);

struct _LARGE_INTEGER {
    long long QuadPart;
};
using LARGE_INTEGER = _LARGE_INTEGER;
DWORD timeGetTime();

struct BITMAPV4HEADER {
    uint32_t bV4Size;
    int32_t bV4Width, bV4Height;
    uint16_t bV4Planes, bV4BitCount;
    uint32_t bV4V4Compression;
    uint32_t bV4SizeImage;
    int32_t bV4XPelsPerMeter, bV4YPelsPerMeter;
    uint32_t bV4ClrUsed, bV4ClrImportant;
    uint32_t bV4RedMask, bV4GreenMask, bV4BlueMask, bV4AlphaMask;
    uint32_t bV4CSType;
    uint32_t bV4Endpoints[9];
    uint32_t bV4GammaRed, bV4GammaGreen, bV4GammaBlue;
};
struct BITMAPINFO { BITMAPINFOHEADER bmiHeader; uint32_t bmiColors[256]; };
using LPBITMAPINFO = BITMAPINFO*;
constexpr uint32_t BI_RGB = 0;
constexpr uint32_t BI_BITFIELDS = 3;
constexpr uint32_t DIB_RGB_COLORS = 0;
inline int GetDIBits(HDC, HBITMAP, uint32_t, uint32_t, void*, LPBITMAPINFO, uint32_t) { return 0; }
inline int DeleteDC(HDC) { return 1; }
inline int DeleteObject(void*) { return 1; }
struct MgDib { std::vector<uint8_t> bits; };
inline void* CreateDIBSection(HDC, const BITMAPINFO* info, uint32_t, void** bits, void*, uint32_t) {
    auto* dib = new MgDib();
    int32_t w = 1, h = 1;
    uint16_t bpp = 32;
    if (info) {
        w = info->bmiHeader.biWidth > 0 ? info->bmiHeader.biWidth : 1;
        h = info->bmiHeader.biHeight < 0 ? -info->bmiHeader.biHeight : info->bmiHeader.biHeight;
        if (h <= 0) h = 1;
        if (info->bmiHeader.biBitCount) bpp = info->bmiHeader.biBitCount;
    }
    uint32_t stride = (uint32_t(w) * (bpp / 8) + 3u) & ~3u;
    dib->bits.assign(stride * uint32_t(h), 0);
    if (bits) *bits = dib->bits.data();
    return dib;
}
#ifndef CopyMemory
#define CopyMemory(d,s,n) std::memcpy((d),(s),(n))
#endif
