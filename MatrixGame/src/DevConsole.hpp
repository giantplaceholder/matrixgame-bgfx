// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#ifndef DEV_CONSOLE_INCLUDE
#define DEV_CONSOLE_INCLUDE

typedef void (*CMD_HANDLER)(const std::wstring& cmd, const std::wstring& params);

struct SCmdItem
{
    const wchar *cmd;
    CMD_HANDLER handler;
};

#define DCON_ACTIVE SETBIT(0)
#define DCON_CURSOR SETBIT(1)
#define DCON_SHIFT  SETBIT(2)

#define DEV_CONSOLE_CURSOR_FLASH_PERIOD 300

class CDevConsole : public CMain {
    DWORD m_Flags;

    static SCmdItem m_Commands[];

    std::wstring m_Text;
    int m_CurPos;

    int m_NextTime;
    int m_Time;

public:
    CDevConsole(void);
    ~CDevConsole();

    bool IsActive(void) const { return FLAG(m_Flags, DCON_ACTIVE); }
    void SetActive(bool active);

    void ShowHelp(void);

    void Takt(int ms);
    void Keyboard(uint8_t vk, bool down);
};

#endif