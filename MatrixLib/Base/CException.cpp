// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "CException.hpp"
#include <stdio.h>

#include "Tracer.hpp"

#include <utils.hpp>

namespace Base {

CException::CException(const char *file, int line)
: m_File{file}
, m_Line{line}
, call_trace{generate_trace_text()}
{
}

std::wstring CException::Info() const
{
    return
        utils::format(
            L"%lsFile=%ls\nLine=%d\n",
            utils::to_wstring(call_trace).c_str(),
            utils::to_wstring(m_File).c_str(),
            m_Line);
}

CExceptionStr::CExceptionStr(
    const char *file,
    int line,
    const wchar_t *str,
    const wchar_t *str2)
: CException(file, line)
{
    m_str = str ? str : L"";
    if (str2) m_str += str2;
}

std::wstring CExceptionStr::Info() const
{
    return CException::Info() + L"Text: " + m_str.c_str();
}

}  // namespace Base
