//-*-c++-*-
#pragma once
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/native_widget_types.h"

class SkBitmap;

namespace content {
class ColorChooser;
class WebContents;
}

namespace meson {
// Shows a color chooser that reports to the given WebContents.
content::ColorChooser* ShowColorChooser(content::WebContents* web_contents, SkColor initial_color);
}
