// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#ifdef BASE_DLL
#ifdef BASE_EXPORTS
#define BASE_API __declspec(dllexport)
#else
#define BASE_API __declspec(dllimport)
#endif
#else
#define BASE_API
#endif

// ////////////////////////
#include <cstdint>
using wchar = wchar_t;
using dword = uint32_t;
// ////////////////////////

#define IS_UNICODE() true

#pragma warning (disable : 4201)
#pragma warning (disable : 4238)

namespace Base {

// lint -e1401

class CPoint {
public:
    int x, y;

    CPoint() {}
    CPoint(int ax, int ay) {
        x = ax;
        y = ay;
    }

    bool operator==(const CPoint &zn) const { return (x == zn.x) && (y == zn.y); }
    CPoint &operator+=(const CPoint &zn) {
        x += zn.x;
        y += zn.y;
        return *this;
    }
    CPoint &operator-=(const CPoint &zn) {
        x -= zn.x;
        y -= zn.y;
        return *this;
    }

    int Dist2(const CPoint &p) const { return (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y); }
};

class CRect {
public:
    int left, top, right, bottom;

    CRect() {}
    CRect(int _left, int _top, int _right, int _bottom) {
        left = _left;
        top = _top;
        right = _right;
        bottom = _bottom;
    }

    bool IsEmpty(void) const { return (right <= left) || (bottom <= top); }
    bool IsInRect(const CPoint &pos) const { return (left < pos.x && top < pos.y && right > pos.x && bottom > pos.y); }

    void Normalize(void) {
        if (left > right) {
            left ^= right;
            right ^= left;
            left ^= right;
        }
        if (top > bottom) {
            top ^= bottom;
            bottom ^= top;
            top ^= bottom;
        }
    }
};

// lint +e1401
}  // namespace Base

using namespace Base; // TODO: this namespace is not needed, remove it

typedef wchar_t wchar;
typedef unsigned char byte;
typedef unsigned short word;
typedef uint32_t dword;
typedef uint64_t int64;
typedef unsigned int uint;

#define LIST_ADD(el, first, last, prev, next) \
    {                                         \
        if (last != nullptr) {                   \
            last->next = el;                  \
        }                                     \
        el->prev = last;                      \
        el->next = nullptr;                      \
        last = el;                            \
        if (first == nullptr) {                  \
            first = el;                       \
        }                                     \
    }
#define LIST_ADD_FIRST(el, first, last, prev, next) \
    {                                               \
        if (first != nullptr) {                        \
            first->prev = el;                       \
        }                                           \
        el->next = first;                           \
        el->prev = nullptr;                            \
        first = el;                                 \
        if (last == nullptr) {                         \
            last = el;                              \
        }                                           \
    }
#define LIST_INSERT(perel, el, first, last, prev, next) \
    {                                                   \
        if (perel == nullptr) {                            \
            LIST_ADD(el, first, last, prev, next);      \
        }                                               \
        else {                                          \
            el->prev = perel->prev;                     \
            el->next = perel;                           \
            if (perel->prev != nullptr) {                  \
                perel->prev->next = el;                 \
            }                                           \
            perel->prev = el;                           \
            if (perel == first) {                       \
                first = el;                             \
            }                                           \
        }                                               \
    }

#define LIST_DEL(el, first, last, prev, next) \
    {                                         \
        if (el->prev != nullptr)                 \
            el->prev->next = el->next;        \
        if (el->next != nullptr)                 \
            el->next->prev = el->prev;        \
        if (last == el)                       \
            last = el->prev;                  \
        if (first == el)                      \
            first = el->next;                 \
    }

#define LIST_DEL_CLEAR(el, first, last, prev, next) \
    {                                               \
        if (el->prev != nullptr)                       \
            el->prev->next = el->next;              \
        if (el->next != nullptr)                       \
            el->next->prev = el->prev;              \
        if (last == el)                             \
            last = el->prev;                        \
        if (first == el)                            \
            first = el->next;                       \
        el->prev = nullptr;                            \
        el->next = nullptr;                            \
    }

#define SETBIT(x)           (((dword)1) << x)
#define SETFLAG(f, mask)    f |= (mask)
#define RESETFLAG(f, mask)  f &= ~(mask)
#define INVERTFLAG(f, mask) f ^= (mask)
#define FLAG(f, mask)       ((f & (mask)) != 0)
#define INITFLAG(f, mask, val) \
    if (val) {                 \
        SETFLAG(f, mask);      \
    }                          \
    else {                     \
        RESETFLAG(f, mask);    \
    }

// portable debugbreak definition
#ifdef _MSC_VER
#define debugbreak __debugbreak
#else
static inline void debugbreak(void) {
    asm volatile("int $0x03");
}
#endif // _MSC_VER