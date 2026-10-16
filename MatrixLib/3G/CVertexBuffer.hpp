// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#ifndef CVERTEXBUFFER_H
#define CVERTEXBUFFER_H

#include "d3dx9.h"
#include "CCoordSystem.hpp"
#include "SVertex.hpp"

class CVertexBuffer : public CWorldObject {
    int _vsize;        // one vertex size
    int _count_verts;  // количество вершин в буфере
    IDirect3DVertexBuffer9 *_buffer;

public:
    CVertexBuffer(CCoordSystem *cs, int vertex_size)
      : CWorldObject(cs), _vsize(vertex_size), _buffer(0), _count_verts(0) {}

    void CreateBuffer(int count_verts) {
        _count_verts = count_verts;
        if (_buffer)
            _buffer->Release();

        g_D3DD->CreateVertexBuffer(count_verts * _vsize, D3DUSAGE_POINTS | D3DUSAGE_WRITEONLY,
                                   D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1, D3DPOOL_DEFAULT, &_buffer, 0);
    }

    void *Lock(void) {
        void *out;
        ASSERT_DX(_buffer->Lock(0, 0, &out, 0));
        return out;
    }

    void Unlock(void) { ASSERT_DX(_buffer->Unlock()); }

    virtual void Render(void) {}
};

#endif