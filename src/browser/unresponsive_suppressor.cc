#include "browser/unresponsive_suppressor.h"

namespace meson {

namespace {
int g_suppress_level = 0;
}

bool IsUnresponsiveEventSuppressed() {
  return g_suppress_level > 0;
}

UnresponsiveSuppressor::UnresponsiveSuppressor() {
  g_suppress_level++;
}

UnresponsiveSuppressor::~UnresponsiveSuppressor() {
  g_suppress_level--;
}
}
