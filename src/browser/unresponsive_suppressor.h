//-*-c++-*-
#pragma once

#include "base/macros.h"

namespace meson {
bool IsUnresponsiveEventSuppressed();

class UnresponsiveSuppressor {
 public:
  UnresponsiveSuppressor();
  ~UnresponsiveSuppressor();

 private:
  DISALLOW_COPY_AND_ASSIGN(UnresponsiveSuppressor);
};

}  // namespace atom
