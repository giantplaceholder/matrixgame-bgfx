// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "Interface.h"
#include "CIFaceElement.h"

////////////////////////////////////////////////////////
// Image container class

class CIFaceImage : public CMain {
public:
    CTextureManaged *m_Image;
    std::wstring m_strName;
    float m_xTexPos;
    float m_yTexPos;
    float m_TexWidth;
    float m_TexHeight;
    float m_Width;
    float m_Height;
    IFaceElementType m_Type;
    CIFaceImage *m_PrevImage, *m_NextImage;

    CIFaceImage();
    ~CIFaceImage();
};