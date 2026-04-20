// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//
// BuiltinGlyphs.cpp - GDI version
// Direct geometric rendering for Box Drawing and Block Elements characters.
// Based on AtlasEngine's BuiltinGlyphs.cpp, adapted for GDI.
//
// NOTE: Shade characters (░▒▓ U+2591-U+2593) are NOT handled here because
// GDI FillRect cannot produce proper dithered patterns. They use font rendering.

#include "precomp.h"
#include "BuiltinGlyphs.h"

#pragma warning(disable : 26429)
#pragma warning(disable : 26446)
#pragma warning(disable : 26472)
#pragma warning(disable : 26481)
#pragma warning(disable : 26482)

namespace Microsoft::Console::Render::GdiBuiltinGlyphs
{

union Instruction
{
    struct
    {
        uint32_t shape : 4;
        uint32_t begX : 5;
        uint32_t begY : 5;
        uint32_t endX : 5;
        uint32_t endY : 5;
    };
    uint32_t value = 0;
};
static_assert(sizeof(Instruction) == sizeof(uint32_t));

static constexpr uint32_t InstructionsPerGlyph = 4;

enum Shape : uint32_t
{
    Shape_Filled025,    // 25% shade (░ light shade)
    Shape_Filled050,    // 50% shade (▒ medium shade)
    Shape_Filled075,    // 75% shade (▓ dark shade)
    Shape_Filled100,    // Solid filled rectangle
    Shape_LightLine,    // Thin line (1/6 cell width)
    Shape_HeavyLine,    // Thick line (1/3 cell width)
    Shape_EmptyRect,    // Hollow rectangle outline
    Shape_RoundRect,    // Hollow rounded rectangle
    Shape_FilledEllipsis,
    Shape_EmptyEllipsis,
    Shape_ClosedFilledPath,
    Shape_OpenLinePath,
};

enum Pos : uint32_t
{
    Pos_Min,
    Pos_Max,

    Pos_0_1,
    Pos_0_1_Add_0_5,
    Pos_1_1,
    Pos_1_1_Sub_0_5,

    Pos_1_2,
    Pos_1_2_Sub_0_5,
    Pos_1_2_Add_0_5,
    Pos_1_2_Sub_1,
    Pos_1_2_Add_1,

    Pos_1_4,
    Pos_3_4,

    Pos_2_6,
    Pos_3_6,
    Pos_5_6,

    Pos_1_8,
    Pos_3_8,
    Pos_5_8,
    Pos_7_8,

    Pos_2_9,
    Pos_3_9,
    Pos_5_9,
    Pos_6_9,
    Pos_8_9,

    Pos_2_12,
    Pos_3_12,
    Pos_5_12,
    Pos_6_12,
    Pos_8_12,
    Pos_9_12,
    Pos_11_12,
};

inline constexpr float Pos_Lut[][2] = {
    /* Pos_Min         */ { -0.5f, 0.0f },
    /* Pos_Max         */ { 1.5f, 0.0f },

    /* Pos_0_1         */ { 0.0f, 0.0f },
    /* Pos_0_1_Add_0_5 */ { 0.0f, 0.5f },
    /* Pos_1_1         */ { 1.0f, 0.0f },
    /* Pos_1_1_Sub_0_5 */ { 1.0f, -0.5f },

    /* Pos_1_2         */ { 1.0f / 2.0f, 0.0f },
    /* Pos_1_2_Sub_0_5 */ { 1.0f / 2.0f, -0.5f },
    /* Pos_1_2_Add_0_5 */ { 1.0f / 2.0f, 0.5f },
    /* Pos_1_2_Sub_1   */ { 1.0f / 2.0f, -1.0f },
    /* Pos_1_2_Add_1   */ { 1.0f / 2.0f, 1.0f },

    /* Pos_1_4         */ { 1.0f / 4.0f, 0.0f },
    /* Pos_3_4         */ { 3.0f / 4.0f, 0.0f },

    /* Pos_2_6         */ { 2.0f / 6.0f, 0.0f },
    /* Pos_3_6         */ { 3.0f / 6.0f, 0.0f },
    /* Pos_5_6         */ { 5.0f / 6.0f, 0.0f },

    /* Pos_1_8         */ { 1.0f / 8.0f, 0.0f },
    /* Pos_3_8         */ { 3.0f / 8.0f, 0.0f },
    /* Pos_5_8         */ { 5.0f / 8.0f, 0.0f },
    /* Pos_7_8         */ { 7.0f / 8.0f, 0.0f },

    /* Pos_2_9         */ { 2.0f / 9.0f, 0.0f },
    /* Pos_3_9         */ { 3.0f / 9.0f, 0.0f },
    /* Pos_5_9         */ { 5.0f / 9.0f, 0.0f },
    /* Pos_6_9         */ { 6.0f / 9.0f, 0.0f },
    /* Pos_8_9         */ { 8.0f / 9.0f, 0.0f },

    /* Pos_2_12        */ { 2.0f / 12.0f, 0.0f },
    /* Pos_3_12        */ { 3.0f / 12.0f, 0.0f },
    /* Pos_5_12        */ { 5.0f / 12.0f, 0.0f },
    /* Pos_6_12        */ { 6.0f / 12.0f, 0.0f },
    /* Pos_8_12        */ { 8.0f / 12.0f, 0.0f },
    /* Pos_9_12        */ { 9.0f / 12.0f, 0.0f },
    /* Pos_11_12       */ { 11.0f / 12.0f, 0.0f },
};

// ============================================================================
// BoxDrawing table - EXACT COPY from AtlasEngine
// Excludes shade characters (U+2591-U+2593) which need font rendering
// ============================================================================

static constexpr Instruction BoxDrawing[BoxDrawing_CharCount][InstructionsPerGlyph] = {
    // U+2500 ─ BOX DRAWINGS LIGHT HORIZONTAL
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 } },
    // U+2501 ━ BOX DRAWINGS HEAVY HORIZONTAL
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 } },
    // U+2502 │ BOX DRAWINGS LIGHT VERTICAL
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+2503 ┃ BOX DRAWINGS HEAVY VERTICAL
    { Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+2504 ┄ BOX DRAWINGS LIGHT TRIPLE DASH HORIZONTAL
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_2_9, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_3_9, Pos_1_2, Pos_5_9, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_6_9, Pos_1_2, Pos_8_9, Pos_1_2 } },
    // U+2505 ┅ BOX DRAWINGS HEAVY TRIPLE DASH HORIZONTAL
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_2_9, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_3_9, Pos_1_2, Pos_5_9, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_6_9, Pos_1_2, Pos_8_9, Pos_1_2 } },
    // U+2506 ┆ BOX DRAWINGS LIGHT TRIPLE DASH VERTICAL
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_2_9 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_3_9, Pos_1_2, Pos_5_9 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_6_9, Pos_1_2, Pos_8_9 } },
    // U+2507 ┇ BOX DRAWINGS HEAVY TRIPLE DASH VERTICAL
    { Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_2_9 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_3_9, Pos_1_2, Pos_5_9 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_6_9, Pos_1_2, Pos_8_9 } },
    // U+2508 ┈ BOX DRAWINGS LIGHT QUADRUPLE DASH HORIZONTAL
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_2_12, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_3_12, Pos_1_2, Pos_5_12, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_6_12, Pos_1_2, Pos_8_12, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_9_12, Pos_1_2, Pos_11_12, Pos_1_2 } },
    // U+2509 ┉ BOX DRAWINGS HEAVY QUADRUPLE DASH HORIZONTAL
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_2_12, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_3_12, Pos_1_2, Pos_5_12, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_6_12, Pos_1_2, Pos_8_12, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_9_12, Pos_1_2, Pos_11_12, Pos_1_2 } },
    // U+250A ┊ BOX DRAWINGS LIGHT QUADRUPLE DASH VERTICAL
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_2_12 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_3_12, Pos_1_2, Pos_5_12 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_6_12, Pos_1_2, Pos_8_12 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_9_12, Pos_1_2, Pos_11_12 } },
    // U+250B ┋ BOX DRAWINGS HEAVY QUADRUPLE DASH VERTICAL
    { Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_2_12 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_3_12, Pos_1_2, Pos_5_12 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_6_12, Pos_1_2, Pos_8_12 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_9_12, Pos_1_2, Pos_11_12 } },
    // U+250C ┌ BOX DRAWINGS LIGHT DOWN AND RIGHT
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_0_5, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+250D ┍ BOX DRAWINGS DOWN LIGHT AND RIGHT HEAVY
    { Instruction{ Shape_HeavyLine, Pos_1_2_Sub_0_5, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+250E ┎ BOX DRAWINGS DOWN HEAVY AND RIGHT LIGHT
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+250F ┏ BOX DRAWINGS HEAVY DOWN AND RIGHT
    { Instruction{ Shape_HeavyLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2510 ┐ BOX DRAWINGS LIGHT DOWN AND LEFT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_0_5, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2511 ┑ BOX DRAWINGS DOWN LIGHT AND LEFT HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_0_5, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2512 ┒ BOX DRAWINGS DOWN HEAVY AND LEFT LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2513 ┓ BOX DRAWINGS HEAVY DOWN AND LEFT
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2514 └ BOX DRAWINGS LIGHT UP AND RIGHT
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_0_5, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2515 ┕ BOX DRAWINGS UP LIGHT AND RIGHT HEAVY
    { Instruction{ Shape_HeavyLine, Pos_1_2_Sub_0_5, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2516 ┖ BOX DRAWINGS UP HEAVY AND RIGHT LIGHT
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2517 ┗ BOX DRAWINGS HEAVY UP AND RIGHT
    { Instruction{ Shape_HeavyLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2518 ┘ BOX DRAWINGS LIGHT UP AND LEFT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_0_5, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2519 ┙ BOX DRAWINGS UP LIGHT AND LEFT HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_0_5, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+251A ┚ BOX DRAWINGS UP HEAVY AND LEFT LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+251B ┛ BOX DRAWINGS HEAVY UP AND LEFT
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+251C ├ BOX DRAWINGS LIGHT VERTICAL AND RIGHT
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+251D ┝ BOX DRAWINGS VERTICAL LIGHT AND RIGHT HEAVY
    { Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+251E ┞ BOX DRAWINGS UP HEAVY AND RIGHT DOWN LIGHT
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+251F ┟ BOX DRAWINGS DOWN HEAVY AND RIGHT UP LIGHT
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2520 ┠ BOX DRAWINGS VERTICAL HEAVY AND RIGHT LIGHT
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+2521 ┡ BOX DRAWINGS DOWN LIGHT AND RIGHT UP HEAVY
    { Instruction{ Shape_HeavyLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2522 ┢ BOX DRAWINGS UP LIGHT AND RIGHT DOWN HEAVY
    { Instruction{ Shape_HeavyLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2523 ┣ BOX DRAWINGS HEAVY VERTICAL AND RIGHT
    { Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+2524 ┤ BOX DRAWINGS LIGHT VERTICAL AND LEFT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+2525 ┥ BOX DRAWINGS VERTICAL LIGHT AND LEFT HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+2526 ┦ BOX DRAWINGS UP HEAVY AND LEFT DOWN LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2527 ┧ BOX DRAWINGS DOWN HEAVY AND LEFT UP LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2528 ┨ BOX DRAWINGS VERTICAL HEAVY AND LEFT LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+2529 ┩ BOX DRAWINGS DOWN LIGHT AND LEFT UP HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+252A ┪ BOX DRAWINGS UP LIGHT AND LEFT DOWN HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+252B ┫ BOX DRAWINGS HEAVY VERTICAL AND LEFT
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+252C ┬ BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+252D ┭ BOX DRAWINGS LEFT HEAVY AND RIGHT DOWN LIGHT
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_0_5, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+252E ┮ BOX DRAWINGS RIGHT HEAVY AND LEFT DOWN LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2_Sub_0_5, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+252F ┯ BOX DRAWINGS DOWN LIGHT AND HORIZONTAL HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2530 ┰ BOX DRAWINGS DOWN HEAVY AND HORIZONTAL LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2531 ┱ BOX DRAWINGS RIGHT LIGHT AND LEFT DOWN HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2532 ┲ BOX DRAWINGS LEFT LIGHT AND RIGHT DOWN HEAVY
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2533 ┳ BOX DRAWINGS HEAVY DOWN AND HORIZONTAL
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2534 ┴ BOX DRAWINGS LIGHT UP AND HORIZONTAL
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2535 ┵ BOX DRAWINGS LEFT HEAVY AND RIGHT UP LIGHT
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_0_5, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2536 ┶ BOX DRAWINGS RIGHT HEAVY AND LEFT UP LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2_Sub_0_5, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2537 ┷ BOX DRAWINGS UP LIGHT AND HORIZONTAL HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2538 ┸ BOX DRAWINGS UP HEAVY AND HORIZONTAL LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2539 ┹ BOX DRAWINGS RIGHT LIGHT AND LEFT UP HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+253A ┺ BOX DRAWINGS LEFT LIGHT AND RIGHT UP HEAVY
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+253B ┻ BOX DRAWINGS HEAVY UP AND HORIZONTAL
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+253C ┼ BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+253D ┽ BOX DRAWINGS LEFT HEAVY AND RIGHT VERTICAL LIGHT
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+253E ┾ BOX DRAWINGS RIGHT HEAVY AND LEFT VERTICAL LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+253F ┿ BOX DRAWINGS VERTICAL LIGHT AND HORIZONTAL HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+2540 ╀ BOX DRAWINGS UP HEAVY AND DOWN HORIZONTAL LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2541 ╁ BOX DRAWINGS DOWN HEAVY AND UP HORIZONTAL LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2542 ╂ BOX DRAWINGS VERTICAL HEAVY AND HORIZONTAL LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+2543 ╃ BOX DRAWINGS LEFT UP HEAVY AND RIGHT DOWN LIGHT
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2544 ╄ BOX DRAWINGS RIGHT UP HEAVY AND LEFT DOWN LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2545 ╅ BOX DRAWINGS LEFT DOWN HEAVY AND RIGHT UP LIGHT
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2546 ╆ BOX DRAWINGS RIGHT DOWN HEAVY AND LEFT UP LIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2547 ╇ BOX DRAWINGS DOWN LIGHT AND UP HORIZONTAL HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2548 ╈ BOX DRAWINGS UP LIGHT AND DOWN HORIZONTAL HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2549 ╉ BOX DRAWINGS RIGHT LIGHT AND LEFT VERTICAL HEAVY
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+254A ╊ BOX DRAWINGS LEFT LIGHT AND RIGHT VERTICAL HEAVY
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+254B ╋ BOX DRAWINGS HEAVY VERTICAL AND HORIZONTAL
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+254C ╌ BOX DRAWINGS LIGHT DOUBLE DASH HORIZONTAL
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_2_6, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_3_6, Pos_1_2, Pos_5_6, Pos_1_2 } },
    // U+254D ╍ BOX DRAWINGS HEAVY DOUBLE DASH HORIZONTAL
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_2_6, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_3_6, Pos_1_2, Pos_5_6, Pos_1_2 } },
    // U+254E ╎ BOX DRAWINGS LIGHT DOUBLE DASH VERTICAL
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_2_6 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_3_6, Pos_1_2, Pos_5_6 } },
    // U+254F ╏ BOX DRAWINGS HEAVY DOUBLE DASH VERTICAL
    { Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_2_6 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_3_6, Pos_1_2, Pos_5_6 } },
    // U+2550 ═ BOX DRAWINGS DOUBLE HORIZONTAL
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Sub_1, Pos_1_1, Pos_1_2_Sub_1 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Add_1, Pos_1_1, Pos_1_2_Add_1 } },
    // U+2551 ║ BOX DRAWINGS DOUBLE VERTICAL
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_0_1, Pos_1_2_Sub_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_1_2_Add_1, Pos_0_1, Pos_1_2_Add_1, Pos_1_1 } },
    // U+2552 ╒ BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_0_5, Pos_1_2_Sub_1, Pos_1_1, Pos_1_2_Sub_1 },
      Instruction{ Shape_LightLine, Pos_1_2_Sub_0_5, Pos_1_2_Add_1, Pos_1_1, Pos_1_2_Add_1 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1 } },
    // U+2553 ╓ BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_1_2_Sub_0_5, Pos_1_2_Sub_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_1_2_Add_1, Pos_1_2_Sub_0_5, Pos_1_2_Add_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 } },
    // U+2554 ╔ BOX DRAWINGS DOUBLE DOWN AND RIGHT
    { Instruction{ Shape_EmptyRect, Pos_1_2_Sub_1, Pos_1_2_Sub_1, Pos_Max, Pos_Max },
      Instruction{ Shape_EmptyRect, Pos_1_2_Add_1, Pos_1_2_Add_1, Pos_Max, Pos_Max } },
    // U+2555 ╕ BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Sub_1, Pos_1_2_Add_0_5, Pos_1_2_Sub_1 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Add_1, Pos_1_2_Add_0_5, Pos_1_2_Add_1 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1 } },
    // U+2556 ╖ BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_1_2_Sub_0_5, Pos_1_2_Sub_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_1_2_Add_1, Pos_1_2_Sub_0_5, Pos_1_2_Add_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 } },
    // U+2557 ╗ BOX DRAWINGS DOUBLE DOWN AND LEFT
    { Instruction{ Shape_EmptyRect, Pos_Min, Pos_1_2_Sub_1, Pos_1_2_Add_1, Pos_Max },
      Instruction{ Shape_EmptyRect, Pos_Min, Pos_1_2_Add_1, Pos_1_2_Sub_1, Pos_Max } },
    // U+2558 ╘ BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_0_5, Pos_1_2_Sub_1, Pos_1_1, Pos_1_2_Sub_1 },
      Instruction{ Shape_LightLine, Pos_1_2_Sub_0_5, Pos_1_2_Add_1, Pos_1_1, Pos_1_2_Add_1 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2_Add_1 } },
    // U+2559 ╙ BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_0_1, Pos_1_2_Sub_1, Pos_1_2_Add_0_5 },
      Instruction{ Shape_LightLine, Pos_1_2_Add_1, Pos_0_1, Pos_1_2_Add_1, Pos_1_2_Add_0_5 },
      Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 } },
    // U+255A ╚ BOX DRAWINGS DOUBLE UP AND RIGHT
    { Instruction{ Shape_EmptyRect, Pos_1_2_Sub_1, Pos_Min, Pos_Max, Pos_1_2_Add_1 },
      Instruction{ Shape_EmptyRect, Pos_1_2_Add_1, Pos_Min, Pos_Max, Pos_1_2_Sub_1 } },
    // U+255B ╛ BOX DRAWINGS UP SINGLE AND LEFT DOUBLE
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Sub_1, Pos_1_2_Add_0_5, Pos_1_2_Sub_1 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Add_1, Pos_1_2_Add_0_5, Pos_1_2_Add_1 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2_Add_1 } },
    // U+255C ╜ BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_0_1, Pos_1_2_Sub_1, Pos_1_2_Add_0_5 },
      Instruction{ Shape_LightLine, Pos_1_2_Add_1, Pos_0_1, Pos_1_2_Add_1, Pos_1_2_Add_0_5 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 } },
    // U+255D ╝ BOX DRAWINGS DOUBLE UP AND LEFT
    { Instruction{ Shape_EmptyRect, Pos_Min, Pos_Min, Pos_1_2_Add_1, Pos_1_2_Add_1 },
      Instruction{ Shape_EmptyRect, Pos_Min, Pos_Min, Pos_1_2_Sub_1, Pos_1_2_Sub_1 } },
    // U+255E ╞ BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2_Sub_1, Pos_1_1, Pos_1_2_Sub_1 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2_Add_1, Pos_1_1, Pos_1_2_Add_1 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+255F ╟ BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
    { Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_0_1, Pos_1_2_Sub_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_1_2_Add_1, Pos_0_1, Pos_1_2_Add_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1, Pos_1_2 } },
    // U+2560 ╠ BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
    { Instruction{ Shape_EmptyRect, Pos_1_2_Sub_1, Pos_1_2_Sub_1, Pos_Max, Pos_Max },
      Instruction{ Shape_EmptyRect, Pos_1_2_Add_1, Pos_1_2_Add_1, Pos_Max, Pos_Max } },
    // U+2561 ╡ BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Sub_1, Pos_1_2, Pos_1_2_Sub_1 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Add_1, Pos_1_2, Pos_1_2_Add_1 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+2562 ╢ BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Sub_1, Pos_1_2_Sub_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Add_1, Pos_1_2_Add_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_2 } },
    // U+2563 ╣ BOX DRAWINGS DOUBLE VERTICAL AND LEFT
    { Instruction{ Shape_EmptyRect, Pos_Min, Pos_1_2_Sub_1, Pos_1_2_Add_1, Pos_Max },
      Instruction{ Shape_EmptyRect, Pos_Min, Pos_1_2_Add_1, Pos_1_2_Sub_1, Pos_Max } },
    // U+2564 ╤ BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Sub_1, Pos_1_1, Pos_1_2_Sub_1 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Add_1, Pos_1_1, Pos_1_2_Add_1 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2_Sub_1, Pos_1_2, Pos_1_1 } },
    // U+2565 ╥ BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_1_2, Pos_1_2_Sub_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_1_2_Add_1, Pos_1_2, Pos_1_2_Add_1, Pos_1_1 } },
    // U+2566 ╦ BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
    { Instruction{ Shape_EmptyRect, Pos_1_2_Sub_1, Pos_1_2_Sub_1, Pos_Max, Pos_Max },
      Instruction{ Shape_EmptyRect, Pos_1_2_Add_1, Pos_1_2_Add_1, Pos_Max, Pos_Max } },
    // U+2567 ╧ BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Sub_1, Pos_1_1, Pos_1_2_Sub_1 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Add_1, Pos_1_1, Pos_1_2_Add_1 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2_Add_1 } },
    // U+2568 ╨ BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_0_1, Pos_1_2_Sub_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2_Add_1, Pos_0_1, Pos_1_2_Add_1, Pos_1_2 } },
    // U+2569 ╩ BOX DRAWINGS DOUBLE UP AND HORIZONTAL
    { Instruction{ Shape_EmptyRect, Pos_1_2_Sub_1, Pos_Min, Pos_Max, Pos_1_2_Add_1 },
      Instruction{ Shape_EmptyRect, Pos_1_2_Add_1, Pos_Min, Pos_Max, Pos_1_2_Sub_1 } },
    // U+256A ╪ BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Sub_1, Pos_1_1, Pos_1_2_Sub_1 },
      Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2_Add_1, Pos_1_1, Pos_1_2_Add_1 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+256B ╫ BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2_Sub_1, Pos_0_1, Pos_1_2_Sub_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_1_2_Add_1, Pos_0_1, Pos_1_2_Add_1, Pos_1_1 } },
    // U+256C ╬ BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
    { Instruction{ Shape_EmptyRect, Pos_1_2_Sub_1, Pos_1_2_Sub_1, Pos_Max, Pos_Max },
      Instruction{ Shape_EmptyRect, Pos_1_2_Add_1, Pos_1_2_Add_1, Pos_Max, Pos_Max } },
    // U+256D ╭ BOX DRAWINGS LIGHT ARC DOWN AND RIGHT
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+256E ╮ BOX DRAWINGS LIGHT ARC DOWN AND LEFT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+256F ╯ BOX DRAWINGS LIGHT ARC UP AND LEFT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2570 ╰ BOX DRAWINGS LIGHT ARC UP AND RIGHT
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2571 ╱ BOX DRAWINGS LIGHT DIAGONAL UPPER RIGHT TO LOWER LEFT
    { Instruction{ Shape_LightLine, Pos_1_1, Pos_0_1, Pos_0_1, Pos_1_1 } },
    // U+2572 ╲ BOX DRAWINGS LIGHT DIAGONAL UPPER LEFT TO LOWER RIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_0_1, Pos_1_1, Pos_1_1 } },
    // U+2573 ╳ BOX DRAWINGS LIGHT DIAGONAL CROSS
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_0_1, Pos_1_1, Pos_1_1 },
      Instruction{ Shape_LightLine, Pos_1_1, Pos_0_1, Pos_0_1, Pos_1_1 } },
    // U+2574 ╴ BOX DRAWINGS LIGHT LEFT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 } },
    // U+2575 ╵ BOX DRAWINGS LIGHT UP
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2576 ╶ BOX DRAWINGS LIGHT RIGHT
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 } },
    // U+2577 ╷ BOX DRAWINGS LIGHT DOWN
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2578 ╸ BOX DRAWINGS HEAVY LEFT
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 } },
    // U+2579 ╹ BOX DRAWINGS HEAVY UP
    { Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+257A ╺ BOX DRAWINGS HEAVY RIGHT
    { Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 } },
    // U+257B ╻ BOX DRAWINGS HEAVY DOWN
    { Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+257C ╼ BOX DRAWINGS LIGHT LEFT AND HEAVY RIGHT
    { Instruction{ Shape_LightLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 } },
    // U+257D ╽ BOX DRAWINGS LIGHT UP AND HEAVY DOWN
    { Instruction{ Shape_LightLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_HeavyLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+257E ╾ BOX DRAWINGS HEAVY LEFT AND LIGHT RIGHT
    { Instruction{ Shape_HeavyLine, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_2 } },
    // U+257F ╿ BOX DRAWINGS HEAVY UP AND LIGHT DOWN
    { Instruction{ Shape_HeavyLine, Pos_1_2, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_LightLine, Pos_1_2, Pos_1_2, Pos_1_2, Pos_1_1 } },

    // ========================================================================
    // BLOCK ELEMENTS (U+2580 - U+259F)
    // NOTE: U+2591-U+2593 (░▒▓ shade chars) are excluded - use font rendering
    // ========================================================================
    // U+2580 ▀ UPPER HALF BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_1, Pos_1_2 } },
    // U+2581 ▁ LOWER ONE EIGHTH BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_7_8, Pos_1_1, Pos_1_1 } },
    // U+2582 ▂ LOWER ONE QUARTER BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_3_4, Pos_1_1, Pos_1_1 } },
    // U+2583 ▃ LOWER THREE EIGHTHS BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_5_8, Pos_1_1, Pos_1_1 } },
    // U+2584 ▄ LOWER HALF BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_1_2, Pos_1_1, Pos_1_1 } },
    // U+2585 ▅ LOWER FIVE EIGHTHS BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_3_8, Pos_1_1, Pos_1_1 } },
    // U+2586 ▆ LOWER THREE QUARTERS BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_1_4, Pos_1_1, Pos_1_1 } },
    // U+2587 ▇ LOWER SEVEN EIGHTHS BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_1_8, Pos_1_1, Pos_1_1 } },
    // U+2588 █ FULL BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_1, Pos_1_1 } },
    // U+2589 ▉ LEFT SEVEN EIGHTHS BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_7_8, Pos_1_1 } },
    // U+258A ▊ LEFT THREE QUARTERS BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_3_4, Pos_1_1 } },
    // U+258B ▋ LEFT FIVE EIGHTHS BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_5_8, Pos_1_1 } },
    // U+258C ▌ LEFT HALF BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_2, Pos_1_1 } },
    // U+258D ▍ LEFT THREE EIGHTHS BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_3_8, Pos_1_1 } },
    // U+258E ▎ LEFT ONE QUARTER BLOCK
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_4, Pos_1_1 } },
    // U+258F ▏ LEFT ONE EIGHTH BLOCK
                { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_8, Pos_1_1 } },
                // U+2590 ▐ RIGHT HALF BLOCK
                { Instruction{ Shape_Filled100, Pos_1_2, Pos_0_1, Pos_1_1, Pos_1_1 } },
                // U+2591 ░ LIGHT SHADE (25% foreground)
                { Instruction{ Shape_Filled025, Pos_0_1, Pos_0_1, Pos_1_1, Pos_1_1 } },
                // U+2592 ▒ MEDIUM SHADE (50% foreground)
                { Instruction{ Shape_Filled050, Pos_0_1, Pos_0_1, Pos_1_1, Pos_1_1 } },
                // U+2593 ▓ DARK SHADE (75% foreground)
                { Instruction{ Shape_Filled075, Pos_0_1, Pos_0_1, Pos_1_1, Pos_1_1 } },
                // U+2594 ▔ UPPER ONE EIGHTH BLOCK
                { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_1, Pos_1_8 } },    // U+2595 ▕ RIGHT ONE EIGHTH BLOCK
    { Instruction{ Shape_Filled100, Pos_7_8, Pos_0_1, Pos_1_1, Pos_1_1 } },
    // U+2596 ▖ QUADRANT LOWER LEFT
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+2597 ▗ QUADRANT LOWER RIGHT
    { Instruction{ Shape_Filled100, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_1 } },
    // U+2598 ▘ QUADRANT UPPER LEFT
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_2, Pos_1_2 } },
    // U+2599 ▙ QUADRANT UPPER LEFT AND LOWER LEFT AND LOWER RIGHT
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_2, Pos_1_1 },
      Instruction{ Shape_Filled100, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_1 } },
    // U+259A ▚ QUADRANT UPPER LEFT AND LOWER RIGHT
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_2, Pos_1_2 },
      Instruction{ Shape_Filled100, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_1 } },
    // U+259B ▛ QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER LEFT
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_Filled100, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_1 } },
    // U+259C ▜ QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER RIGHT
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_0_1, Pos_1_1, Pos_1_2 },
      Instruction{ Shape_Filled100, Pos_1_2, Pos_1_2, Pos_1_1, Pos_1_1 } },
    // U+259D ▝ QUADRANT UPPER RIGHT
    { Instruction{ Shape_Filled100, Pos_1_2, Pos_0_1, Pos_1_1, Pos_1_2 } },
    // U+259E ▞ QUADRANT UPPER RIGHT AND LOWER LEFT
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_1 },
      Instruction{ Shape_Filled100, Pos_1_2, Pos_0_1, Pos_1_1, Pos_1_2 } },
    // U+259F ▟ QUADRANT UPPER RIGHT AND LOWER LEFT AND LOWER RIGHT
    { Instruction{ Shape_Filled100, Pos_0_1, Pos_1_2, Pos_1_2, Pos_1_1 },
      Instruction{ Shape_Filled100, Pos_1_2, Pos_0_1, Pos_1_1, Pos_1_1 } },
};

// ============================================================================
// GDI HELPER FUNCTIONS
// ============================================================================

// Draw shade pattern (25%, 50%, or 75% foreground) using fine grain dithering
// Manually draws pixels for better control over dot size
static void DrawShadeRect(HDC hdc, int x1, int y1, int x2, int y2, COLORREF fgColor, COLORREF bgColor, int shadePercent) noexcept
{
    const int width = x2 - x1;
    const int height = y2 - y1;
    if (width <= 0 || height <= 0) return;
    
    // Create a DIB section for pixel-level control
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // negative = top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* pvBits = nullptr;
    HBITMAP hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
    if (!hbm || !pvBits) return;
    
    DWORD* pPixels = static_cast<DWORD*>(pvBits);
    
    // Convert colors to DWORD format (0x00BBGGRR)
    const DWORD fg = (GetBValue(fgColor) << 16) | (GetGValue(fgColor) << 8) | GetRValue(fgColor);
    const DWORD bg = (GetBValue(bgColor) << 16) | (GetGValue(bgColor) << 8) | GetRValue(bgColor);
    
    // Fill with dithered pattern
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            bool fillFg = false;
            
            switch (shadePercent)
            {
            case 25: // ░ Light shade - uniform sparse pattern (~25%)
                // Use a 4x4 repeating pattern with 4 pixels filled (4/16 = 25%)
                {
                    const int px = x % 4;
                    const int py = y % 4;
                    // Fill pattern: one pixel in each 2x2 sub-grid
                    fillFg = (px == 0 && py == 0) || (px == 2 && py == 2);
                }
                break;
                
            case 50: // ▒ Medium shade - classic checkerboard
                fillFg = ((x ^ y) & 1) != 0;
                break;
                
            case 75: // ▓ Dark shade - inverse of light
                {
                    const int px = x % 4;
                    const int py = y % 4;
                    fillFg = !((px == 0 && py == 0) || (px == 2 && py == 2));
                }
                break;
            }
            
            pPixels[y * width + x] = fillFg ? fg : bg;
        }
    }
    
    // Create a compatible DC and select the bitmap
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmOld = static_cast<HBITMAP>(SelectObject(hdcMem, hbm));
    
    // BitBlt to the target
    BitBlt(hdc, x1, y1, width, height, hdcMem, 0, 0, SRCCOPY);
    
    // Cleanup
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    DeleteObject(hbm);
}

static void DrawFilledRect(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color) noexcept
{
    RECT rect{ x1, y1, x2, y2 };
    wil::unique_hbrush hbr(CreateSolidBrush(color));
    if (hbr)
    {
        FillRect(hdc, &rect, hbr.get());
    }
}

static void DrawLine(HDC hdc, int x1, int y1, int x2, int y2, int width, COLORREF color) noexcept
{
    wil::unique_hpen hpen(CreatePen(PS_SOLID, width, color));
    if (hpen)
    {
        const auto oldPen = SelectPen(hdc, hpen.get());
        MoveToEx(hdc, x1, y1, nullptr);
        LineTo(hdc, x2, y2);
        SelectPen(hdc, oldPen);
    }
}

static void DrawEmptyRect(HDC hdc, int x1, int y1, int x2, int y2, int lineWidth, COLORREF color) noexcept
{
    wil::unique_hpen hpen(CreatePen(PS_SOLID, lineWidth, color));
    wil::unique_hbrush hbr((HBRUSH)GetStockObject(NULL_BRUSH));
    if (hpen && hbr)
    {
        const auto oldPen = SelectPen(hdc, hpen.get());
        const auto oldBrush = SelectBrush(hdc, hbr.get());
        Rectangle(hdc, x1, y1, x2, y2);
        SelectBrush(hdc, oldBrush);
        SelectPen(hdc, oldPen);
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool IsBuiltinGlyph(wchar_t ch) noexcept
{
    return ch >= BoxDrawing_FirstChar && ch <= BoxDrawing_LastChar;
}

bool DrawBuiltinGlyph(HDC hdc, int x, int y, int width, int height, COLORREF fgColor, COLORREF bgColor, wchar_t ch) noexcept
{
    if (!IsBuiltinGlyph(ch))
    {
        return false;
    }

    const size_t index = static_cast<size_t>(ch - BoxDrawing_FirstChar);
    const auto instructions = &BoxDrawing[index][0];

    const float rectX = static_cast<float>(x);
    const float rectY = static_cast<float>(y);
    const float rectW = static_cast<float>(width);
    const float rectH = static_cast<float>(height);

    const float lightLineWidth = std::max(1.0f, roundf(rectW / 6.0f));
    const float heavyLineWidth = lightLineWidth * 2.0f;

    for (size_t i = 0; i < InstructionsPerGlyph; ++i)
    {
        const auto& instruction = instructions[i];
        if (instruction.value == 0)
        {
            break;
        }

        const auto shape = static_cast<Shape>(instruction.shape);
        
        auto begX = Pos_Lut[instruction.begX][0] * rectW;
        auto begY = Pos_Lut[instruction.begY][0] * rectH;
        auto endX = Pos_Lut[instruction.endX][0] * rectW;
        auto endY = Pos_Lut[instruction.endY][0] * rectH;

        begX += Pos_Lut[instruction.begX][1] * lightLineWidth;
        begY += Pos_Lut[instruction.begY][1] * lightLineWidth;
        endX += Pos_Lut[instruction.endX][1] * lightLineWidth;
        endY += Pos_Lut[instruction.endY][1] * lightLineWidth;

        const float lineWidth = (shape == Shape_HeavyLine) ? heavyLineWidth : lightLineWidth;
        const float lineWidthHalf = lineWidth * 0.5f;
        const bool isHollowRect = (shape == Shape_EmptyRect || shape == Shape_RoundRect);
        const bool isLine = (shape == Shape_LightLine || shape == Shape_HeavyLine);
        const bool isLineX = isLine && begX == endX;
        const bool isLineY = isLine && begY == endY;
        const float lineOffsetX = (isHollowRect || isLineX) ? lineWidthHalf : 0.0f;
        const float lineOffsetY = (isHollowRect || isLineY) ? lineWidthHalf : 0.0f;

        const auto begXabs = rectX + roundf(begX - lineOffsetX) + lineOffsetX;
        const auto begYabs = rectY + roundf(begY - lineOffsetY) + lineOffsetY;
        const auto endXabs = rectX + roundf(endX + lineOffsetX) - lineOffsetX;
        const auto endYabs = rectY + roundf(endY + lineOffsetY) - lineOffsetY;

        const int ix1 = static_cast<int>(begXabs);
        const int iy1 = static_cast<int>(begYabs);
        const int ix2 = static_cast<int>(endXabs);
        const int iy2 = static_cast<int>(endYabs);

        switch (shape)
        {
        case Shape_Filled025:
            // ░ Light shade - traditional sparse dither pattern
            DrawShadeRect(hdc, ix1, iy1, ix2, iy2, fgColor, bgColor, 25);
            break;
        case Shape_Filled050:
            // ▒ Medium shade - traditional checkerboard pattern
            DrawShadeRect(hdc, ix1, iy1, ix2, iy2, fgColor, bgColor, 50);
            break;
        case Shape_Filled075:
            // ▓ Dark shade - traditional dense dither pattern
            DrawShadeRect(hdc, ix1, iy1, ix2, iy2, fgColor, bgColor, 75);
            break;
        case Shape_Filled100:
            DrawFilledRect(hdc, ix1, iy1, ix2, iy2, fgColor);
            break;
        case Shape_LightLine:
            DrawLine(hdc, ix1, iy1, ix2, iy2, static_cast<int>(lightLineWidth), fgColor);
            break;
        case Shape_HeavyLine:
            DrawLine(hdc, ix1, iy1, ix2, iy2, static_cast<int>(heavyLineWidth), fgColor);
            break;
        case Shape_EmptyRect:
        case Shape_RoundRect:
            DrawEmptyRect(hdc, ix1, iy1, ix2, iy2, static_cast<int>(lightLineWidth), fgColor);
            break;
        case Shape_FilledEllipsis:
        case Shape_EmptyEllipsis:
            DrawFilledRect(hdc, ix1, iy1, ix2, iy2, fgColor);
            break;
        case Shape_ClosedFilledPath:
        case Shape_OpenLinePath:
            break;
        }
    }

    return true;
}

} // namespace Microsoft::Console::Render::GdiBuiltinGlyphs
