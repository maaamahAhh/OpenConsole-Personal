// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//
// BuiltinGlyphs.h
//
// This module provides direct geometric rendering for special Unicode characters
// (Box Drawing, Block Elements, etc.) to ensure pixel-perfect alignment without
// anti-aliasing artifacts or gaps that can occur with font rendering.
//
// This is the GDI equivalent of AtlasEngine's BuiltinGlyphs.cpp.

#pragma once

namespace Microsoft::Console::Render::GdiBuiltinGlyphs
{
    // Character ranges we support with direct geometric rendering
    inline constexpr wchar_t BoxDrawing_FirstChar = 0x2500;
    inline constexpr wchar_t BoxDrawing_LastChar = 0x259F;
    inline constexpr size_t BoxDrawing_CharCount = 0xA0; // 160 characters

    // Check if a character should be rendered using builtin glyphs
    bool IsBuiltinGlyph(wchar_t ch) noexcept;

    // Draw a builtin glyph character at the specified position
    // Returns true if the character was drawn, false if not a builtin glyph
    // hdc: The device context to draw on
    // x, y: Top-left position in pixels
    // width, height: Cell size in pixels
    // fgColor: The foreground color to use
    // bgColor: The background color (used for shade patterns)
    // ch: The character to draw
    bool DrawBuiltinGlyph(HDC hdc, int x, int y, int width, int height, COLORREF fgColor, COLORREF bgColor, wchar_t ch) noexcept;
}
