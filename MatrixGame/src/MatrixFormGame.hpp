// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once

#include "Form.hpp"

#include <cstdint>

#define MOUSE_BORDER          4

/**
 * @brief The form(high-level window) which holds the game.
 */
class CFormMatrixGame : public CForm {
private:
    float m_LastWorldX, m_LastWorldY;
    int m_Action;

public:
    CFormMatrixGame(void);
    ~CFormMatrixGame();

    virtual void Enter(void);
    virtual void Leave(void);

    /**
     * @brief Processes the frame effectively redrawing everything on the game containing window.
     */
    virtual void Draw(void);

    /**
     * @brief Processes logical frame of the game containing window.
     *
     * Triggers g_MatrixMap->Takt(step);
     *
     * @param step number of milliseconds since last frame (delta time)
     *
     * TODO: consider renaming russian "Takt" to something consistent like "Update"(Unity) or "Process"(Godot).
     *      also consider renaming step to delta everywhere.
     */
    virtual void Takt(int step);

    /**
     * @brief Game's window mouse movement handler
     *
     * Executed when mouse is moved.
     */
    virtual void MouseMove(int x, int y);

    /**
     * @brief Game's window mouse button press handler.
     *
     * Executed when mouse button is pressed.
     *
     * @param status ButtonStatus: button is either released, pressed down, double clicked or scrolled if it's wheel.
     * @param key Virtual key, see VK_ in "Winuser.h"
     * @param x Cursor X position
     * @param y Cursor Y position
     */
    virtual void MouseKey(ButtonStatus status, int key, int x, int y);

    /**
     * @brief Game's window key press handler.
     *
     * Updates the buttons pressed bitmap record (see input.cpp) and does immediate logic due to key presses (such as
     *      types in Dev console, etc.).
     *
     * Called as a response to a button being pressed originating from the Windows message queue and L3GWndProc.
     *
     * This function gets called before the frame it is going to affect.
     *
     * @param down Is key down?
     * @param vk Virtual key, see VK_ in "Winuser.h"
     */
    virtual void Keyboard(bool down, uint8_t vk);
    virtual void SystemEvent(ESysEvent se);
};
