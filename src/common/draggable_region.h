//-*-c++-*-
#pragma once

#include "ui/gfx/geometry/rect.h"

namespace meson {
struct DraggableRegion {
  bool draggable;
  gfx::Rect bounds;

  DraggableRegion();
};
}
