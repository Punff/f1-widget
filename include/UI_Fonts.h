#pragma once
#include <LovyanGFX.hpp>

// Bring LGFX font types into scope
using lgfx::v1::GFXglyph;
using lgfx::v1::GFXfont;
using lgfx::v1::IFont;

// Include F1 font headers from fonts/ directory
#include "fonts/Formula1_Display_Regular7pt7b.h"
#include "fonts/Formula1_Display_Regular10pt7b.h"
#include "fonts/Formula1_Display_Bold10pt7b.h"
#include "fonts/Formula1_Display_Wide10pt7b.h"

// Centralized Font Aliases
namespace UI {
    namespace Fonts {
        static const IFont* HEADER_BIG   = &Formula1_Display_Wide10pt7b;  // ~28px
        static const IFont* BODY_MAIN    = &Formula1_Display_Regular10pt7b;  // ~18px
        static const IFont* DATA_ACCENT  = &Formula1_Display_Bold10pt7b;  // ~20px bold
        static const IFont* LABEL_SMALL  = &Formula1_Display_Regular7pt7b;  // ~12px
    }
}
