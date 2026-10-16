// MatrixGame - SR2 Planetary battles engine with Windows and Linux support
// Copyright (C) 2004-2019 Elemental Games, Katauri Interactive, CHK-Games
// Copyright (C) 2020-2022 murgesku (https://github.com/murgesku)
// Copyright (C) 2022-2024 vladislavrv (https://github.com/vladislavrv)
// Copyright (C) 2026 giantplaceholder (https://github.com/giantplaceholder)
// Licensed under GPLv2 or any later version
// Refer to the LICENSE and NOTICE files included

#include "MatrixGamePathUtils.hpp"
#include "windows.h"
#include <utils.hpp>
#include <shlobj.h>

std::wstring PathToOutputFiles(const wchar_t *dest)
{
    std::wstring path{};

    if (path.empty()) {
        ITEMIDLIST *pidl;

        HRESULT hRes = SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl);
        if (hRes == NOERROR) {
            wchar_t lpPath[MAX_PATH];
            SHGetPathFromIDListW(pidl, lpPath);

            path = utils::format(L"%ls\\SpaceRangersHD", lpPath);
            CreateDirectoryW(path.c_str(), NULL);
            path += L"\\";
            path += dest;
        }
        else {
            path = utils::format(L".\\%ls", dest);
        }
    }

    return path;
}
