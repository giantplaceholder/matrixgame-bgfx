// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "Form.hpp"
#include "BaseDef.hpp"
#include "Tracer.hpp"

CForm *g_FormFirst;
CForm *g_FormLast;
CForm *g_FormCur;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CForm::CForm() : Base::CMain(), m_Name() {
    DTRACE();

    m_FormPrev = NULL;
    m_FormNext = NULL;

    LIST_ADD(this, g_FormFirst, g_FormLast, m_FormPrev, m_FormNext);
}

CForm::~CForm() {
    DTRACE();

    LIST_DEL(this, g_FormFirst, g_FormLast, m_FormPrev, m_FormNext);
}

void CForm::StaticInit(void) {
    g_FormFirst = NULL;
    g_FormLast = NULL;
    g_FormCur = NULL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void FormChange(CForm *form) {
    DTRACE();

    if (g_FormCur) {
        g_FormCur->Leave();
    }
    g_FormCur = form;
    if (g_FormCur)
        g_FormCur->Enter();
}
