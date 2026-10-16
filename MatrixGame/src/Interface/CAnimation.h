// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

class CIFaceStatic;
class CIFaceElement;

#include "Texture.hpp"

struct SFrame {
    float pos_x;
    float pos_y;
    float pos_z;

    float width;
    float height;

    float tex_width;
    float tex_height;
    float tex_pos_x;
    float tex_pos_y;

    // CWStr name;
    CTextureManaged *tex;
    float ipos_x;
    float ipos_y;
};

class CAnimation : public CMain {
    int m_Period;
    int m_Frames;
    int m_CurrentFrame;
    int m_FramesLoaded;
    int m_TimePass;

    CIFaceStatic *m_FramesBuffer;

public:
    int GetFramesTotal() { return m_Frames; }
    int GetFramesLoaded() { return m_FramesLoaded; }
    int GetCurrentFrameNum() { return m_CurrentFrame; }
    CIFaceElement *GetCurrentFrame();

    void LogicTakt(int ms);
    bool LoadNextFrame(SFrame *frame);

    void RecalcPos(const float &ix, const float &y);

    CAnimation(int frames, int period);
    ~CAnimation();
};