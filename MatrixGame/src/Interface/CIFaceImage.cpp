// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "CIFaceImage.h"

CIFaceImage::CIFaceImage() {
    m_Image = NULL;
    m_PrevImage = NULL;
    m_NextImage = NULL;
    m_xTexPos = 0;
    m_yTexPos = 0;
    m_TexWidth = 0;
    m_TexHeight = 0;
    m_Width = 0;
    m_Height = 0;
    m_strName = L"";
    m_Type = IFACE_IMAGE;
}

CIFaceImage::~CIFaceImage() {}
