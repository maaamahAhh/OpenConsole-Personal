// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "GdiStyleHelper.h"

using namespace Microsoft::Console::Render::Atlas;

// Note: All implementations are provided inline in the header file
// This cpp file is reserved for potential future non-inline implementations

// Design notes:
// 1. All methods are designed as static methods, no instantiation required
// 2. All methods use noexcept marker, guaranteed not to throw exceptions
// 3. All methods have null pointer checks for safety
// 4. Completely replicates GDI window behavior, including:
//    - Window class styles (CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS)
//    - Window styles (WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL)
//    - Extended styles (WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES | WS_EX_APPWINDOW | WS_EX_LAYERED)
//    - Background color management (GWL_CONSOLE_BKCOLOR)
//    - Scrollbar behavior (always visible)
//    - Fullscreen switching logic
