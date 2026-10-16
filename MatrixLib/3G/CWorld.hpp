// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#ifndef CWORLD_H
#define CWORLD_H

#include "d3dx9.h"
#include "CWorldObject.hpp"
#include "CCoordSystem.hpp"
#include "CCamera.hpp"
#include "CVertexBuffer.hpp"

// world
class CWorld : public CWorldObject {
    CCamera *_ccamera;  // current camera

    CWorldObject *_list;

public:
    CWorld(CCoordSystem *cs) : CWorldObject(cs) {}

    virtual void Render(void) {
        // SetTransform
    }

    void Add(CWorldObject *wo) { _list->ListAdd(wo); }

    void SetCam(CCamera *cam) { _ccamera = cam; }
};

#endif