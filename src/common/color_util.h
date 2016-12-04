//-*-c++-*-
#pragma once
#include <string>

#include "third_party/skia/include/core/SkColor.h"

namespace meson {
// Parse hex color like "#FFF" or "#EFEFEF"
SkColor ParseHexColor(const std::string& name);

// Convert color to RGB hex value like "#ABCDEF"
std::string ToRGBHex(SkColor color);
}
