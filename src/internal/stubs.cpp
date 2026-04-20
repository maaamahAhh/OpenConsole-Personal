// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"

#include "../inc/conint.h"
#include <uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "UxTheme.lib")

using namespace Microsoft::Console::Internal;

[[nodiscard]] HRESULT ProcessPolicy::CheckAppModelPolicy(const HANDLE /*hToken*/,
                                                         bool& fIsWrongWayBlocked) noexcept
{
    fIsWrongWayBlocked = false;
    return S_OK;
}

[[nodiscard]] HRESULT ProcessPolicy::CheckIntegrityLevelPolicy(const HANDLE /*hOtherToken*/,
                                                               bool& fIsWrongWayBlocked) noexcept
{
    fIsWrongWayBlocked = false;
    return S_OK;
}

[[nodiscard]] HRESULT Theming::TrySetDarkMode(HWND hwnd) noexcept
{
    if (!hwnd)
    {
        return E_INVALIDARG;
    }

    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            L"SystemUsesLightTheme",
            RRF_RT_DWORD,
            nullptr,
            &value,
            &size) != ERROR_SUCCESS)
    {
        value = 1;  // Default to light theme if registry key not found
    }
    bool const isDarkMode = (value == 0);
    
    BOOL const darkFlag = isDarkMode ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkFlag, sizeof(darkFlag));

    LPCWSTR const themeName = isDarkMode ? L"DarkMode_Explorer" : L"Explorer";
    SetWindowTheme(hwnd, themeName, nullptr);

    return S_OK;
}