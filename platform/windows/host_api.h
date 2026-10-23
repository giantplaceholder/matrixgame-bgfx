// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#pragma once
// real system calls, the game only sees the fake Win32 headers

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MgFindData {
    uint32_t attributes;
    char name[260];
} MgFindData;

void* mgwin_create_file(const wchar_t* name, uint32_t disposition);
int mgwin_read(void* file, void* buffer, uint32_t bytes, uint32_t* read_bytes);
int mgwin_write(void* file, const void* buffer, uint32_t bytes, uint32_t* written);
uint32_t mgwin_size(void* file, uint32_t* high);
uint32_t mgwin_seek(void* file, int32_t distance, uint32_t method);
int mgwin_close(void* file);
void* mgwin_find_first(const wchar_t* pattern, MgFindData* out);
int mgwin_find_next(void* find, MgFindData* out);
int mgwin_find_close(void* find);
int mgwin_qpc(int64_t* out);
int mgwin_qpf(int64_t* out);

int mg_sdl_open(const char* title, int width, int height, int popup);
void mg_sdl_close(void);
void mg_sdl_size(int* width, int* height);
int mg_sdl_poll(uint32_t* message, uintptr_t* wparam, intptr_t* lparam);
const char* mg_sdl_renderer_name(void);
int mgfx_shaders_ok(void);

#ifdef __cplusplus
}
#endif
