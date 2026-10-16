// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include <cstdint>
#include "BaseDef.hpp"

namespace Base {

typedef bool (*REMIND_HANDLER)(uintptr_t param);  // returns true, if core is dead

#define CHECK_TIME 10

/**
 * @brief A timer based callback scheduler, usually used to clean up after resources are no longer needed.
 *
 * Each scheduled callback is represented as an independent SRemindCore object inside a double linked list.
 *
 * It is called as soon as possible(yet with some minimal delay) after you add it initially, but you can postpone it using Use().
 * The way it is used in the game is that Use is called frequently so the callback is only ever called when this constant
 * call of Use() is stopped(i.e. the object is no longer drawn). Usually the free of resources(i.e. textures) is
 * triggered afterwards inside of callback. It is similar to a very simple yet stupid garbage collector.
 *
 * It seems like it does not give you a good guarantee as of exact timing of the callback execution.
 *
 * REMIND_HANDLER handler is pointer to the callback function which should take void* as a parameter and returns bool:
 *                  false if the scheduler has to be deleted or true otherwise.
 */
struct SRemindCore {
    static SRemindCore *first;
    static SRemindCore *last;
    static SRemindCore *current;

    static int gtime; // Global time
    static int ctime; // Checking time (increases in steps)

    SRemindCore *next;
    SRemindCore *prev;
    int time;
    REMIND_HANDLER handler;
    uintptr_t param;

    SRemindCore(REMIND_HANDLER hand, uintptr_t par) : next(nullptr), prev(nullptr), time(gtime), handler(hand), param(par) {}
    ~SRemindCore(void) { Down(); }

    static void StaticInit(void) {
        first = nullptr;
        last = nullptr;
        current = nullptr;
        gtime = 0;
        ctime = 0;
    }

    void Down(void) {
        if (current == this)
            current = this->next;
        LIST_DEL_CLEAR(this, first, last, prev, next);
    }

    /**
     * @brief "touch" this reminder, so the callback's call is postponed for "nexttime".
     *
     * @param nexttime time until callback is called in ms. Can be overridden by another Use().
     */
    void Use(int nexttime) {
        if (first != nullptr) {
            if ((((uintptr_t)next) | ((uintptr_t)prev)) == 0 && (this != first)) {
                LIST_ADD(this, first, last, prev, next);
            }
        }
        else {
            first = this;
            last = this;
            next = nullptr;
            prev = nullptr;
        }
        time = gtime + nexttime;
    }

    static void Takt(int ms);
};

//
//#define REMINDER_TAKT       1000 // one second
//#define REMINDER_MAX_TIME   128 // in REMINDER_TAKT
//
//#define REMINDER_EMPTY      ((DWORD)(-1))
//
// typedef void (*REMINDER_HANDLER)(DWORD uid, DWORD user);
//
// typedef CBuf* PCBuf;
//
// struct SReminderItem
//{
//    SReminderItem *next_free;
//    SReminderItem *prev_free;
//
//    REMINDER_HANDLER handler;
//    DWORD            user;
//
//    int              index0;
//    int              index1;
//    int              time;
//};
//
// class CReminder : public CMain
//{
//    CHeap  *m_Heap;
//    CBuf    m_Items;
//    PCBuf   m_TimeArray[REMINDER_MAX_TIME];
//
//    int     m_Time;
//    int     m_NextTime;
//    int     m_Pointer;
//
//    int     m_Ref;
//
//    SReminderItem *m_FirstFree;
//    SReminderItem *m_LastFree;
//    SReminderItem*  NewItem(void);
//    void    ReleaseItem(SReminderItem *item);
//
//
//    ~CReminder();
//    CReminder(CHeap *heap);
//
//    //void Validate(void);
// public:
//
//    void Clear(void);
//    void Takt(int ms);
//
//    static CReminder* Build(CHeap *heap)
//    {
//        return HNew (heap) CReminder(heap);
//    }
//
//    void Release(void)
//    {
//        //--m_Ref;
//        //if (m_Ref<=0)
//        HDelete(CReminder, this, m_Heap);
//    }
//
//    //void RefInc(void) {++m_Ref;}
//
//
//    bool  RemindOldest(void);
//
//
//    DWORD Create(DWORD id, int in_time, REMINDER_HANDLER handler, DWORD user);
//    void  Delete(DWORD id);
//
//
//};
//
}  // namespace Base
