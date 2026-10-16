// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "CList.hpp"

//===========================================================================//
//                      G L O B A L    F U N C T I O N S

void CList ::Add(

        CList *head) throw() {
    CList *p;

    p = this->m_prev;

    this->m_prev = head->m_prev;

    p->m_next = head;

    head->m_prev = p;
}

//---------------------------------------------------------------------------//

CList *CList ::Sub(

        CList *item) throw() {
    CList *head;
    CList *n;
    CList *p;

    head = this;
    n = item->m_next;
    p = item->m_prev;

    if (head != item) {
        p->m_next = n;
    }
    else {
        head = n;
    }

    if (n != 0) {
        n->m_prev = p;
    }
    else {
        if (head != 0) {
            head->m_prev = p;
        }
    }

    item->Init();

    return head;
}

//---------------------------------------------------------------------------//

void CList ::Split(

        CList *item) throw() {
    CList *p;

    p = item->m_prev;
    item->m_prev = this->m_prev;
    this->m_prev = p;
    p->m_next = 0;
}

//---------------------------------------------------------------------------//

CList *CList ::Insert(

        CList *head, CList *to_item) throw() {
    CList *p;

    if (to_item == 0) {
        this->Add(head);
        return this;
    }

    p = head->m_prev;
    p->m_next = to_item;
    head->m_prev = to_item->m_prev;
    to_item->m_prev = p;

    if (to_item == this) {
        return head;
    }

    return this;
}

//---------------------------------------------------------------------------//

CList *CList ::Move(

        CList *item, CList *to_item) throw() {
    CList *head;

    head = this->Sub(item);
    if (head != 0) {
        return head->Insert(item, to_item);
    }

    return item;
}
