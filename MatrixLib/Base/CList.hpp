// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#ifndef CLIST_HPP
#define CLIST_HPP

class CList {
protected:
    // -------------------------------------------------------//
    //                          D A T A

    CList *m_next;
    CList *m_prev;

    // -------------------------------------------------------//
    //                      M E T H O D S

    inline void Init(void) throw() {
        this->m_next = 0;
        this->m_prev = this;
    }

    // -------------------------------------------------------//

    inline CList(void) throw() { this->Init(); }

    // -------------------------------------------------------//

    inline ~CList(void) throw() {}

    // -------------------------------------------------------//

    void Add(CList *head) throw();

    CList *Sub(CList *item) throw();

    void Split(CList *item) throw();

    CList *Insert(CList *head, CList *to_item) throw();

    CList *Move(CList *item, CList *to_item) throw();
};

// -------------------------------------------------------//

template <class T>
class CListInterface : public CList {
public:
    // -------------------------------------------------------//

    inline T *GetNext(void) const throw() { return static_cast<T *>(this->m_next); }

    // -------------------------------------------------------//

    inline T *GetPrev(void) const throw() { return static_cast<T *>(this->m_prev); }

    // -------------------------------------------------------//

    inline CListInterface(void) throw() : CList() {}

    // -------------------------------------------------------//

    inline ~CListInterface(void) throw() {}

    // -------------------------------------------------------//

    inline void ListAdd(T *head) throw() { this->Add(head); }

    // -------------------------------------------------------//

    inline T *ListSub(T *item) throw() { return static_cast<T *>(this->sub(item)); }

    // -------------------------------------------------------//

    inline void ListSplit(T *item) throw() { this->Split(item); }

    // -------------------------------------------------------//

    inline T *ListInsert(T *head, T *to_item) throw() { return static_cast<T *>(this->Insert(head, to_item)); }

    // -------------------------------------------------------//

    inline T *ListMove(T *item, T *to_item) throw() { return static_cast<T *>(this->Move(item, to_item)); }

    // -------------------------------------------------------//

    void ListFree(void) throw() {
        T *head;
        T *item;

        head = static_cast<T *>(this);

        do {
            item = head->GetNext();

            delete head;

            head = item;
        }
        while (head != NULL);
    }
};

#endif  // #ifndef CLIST_HPP  //
