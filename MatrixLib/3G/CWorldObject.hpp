// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#ifndef CWORLDOBJECT_H
#define CWORLDOBJECT_H

#include "d3dx9.h"

#include "../Base/CList.hpp"

class CCoordSystem;

class CWorldObject : public CMain, public CListInterface<CWorldObject> {
    CCoordSystem *_owner;  // координатная система - владелец

public:
    CWorldObject(CCoordSystem *cs) : _owner(cs) {}

    virtual void Render(void) = 0;
};

#endif