// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include <string>
#include "BaseDef.hpp"

namespace Base {

#define ERROR_E         throw Base::CException(__FILE__, __LINE__)
#define ERROR_S(s)      throw Base::CExceptionStr(__FILE__, __LINE__, s)
#define ERROR_S2(s, s2) throw Base::CExceptionStr(__FILE__, __LINE__, s, s2)

#ifdef ASSERT_OFF
#define ASSERT(zn)
#else
#ifdef _TRACE
#define ASSERT(zn)     \
    {                  \
        if ((zn) == 0) \
            ERROR_E;   \
    }
#else
#define ASSERT(zn)        \
    {                     \
        if ((zn) == 0)    \
            debugbreak(); \
    }
#endif
#endif

// lint -e1712
class CException
{
    const char *m_File;
    int m_Line;

    std::string call_trace;

public:
    CException(const char *file, int line);

    virtual ~CException() = default;

    virtual std::wstring Info(void) const;
};

class BASE_API CExceptionStr : public CException
{
    std::wstring m_str;

public:
    CExceptionStr(const char *file, int line, const wchar *str, const wchar *str2 = NULL);

    CExceptionStr(const char *file, int line, const std::wstring& str)
        :CExceptionStr(file, line, str.c_str())
    {}

    virtual std::wstring Info(void) const;
};
// lint +e1712
}  // namespace Base
