//-*-c++-*-
#pragma once
#include <map>
#include <string>

#include "browser/ui/meson_menu_model.h"
#include "ui/base/accelerators/accelerator.h"

namespace accelerator_util {
typedef struct {
  int position;
  meson::MesonMenuModel* model;
} MenuItem;
typedef std::map<ui::Accelerator, MenuItem> AcceleratorTable;

// Parse a string as an accelerator.
bool StringToAccelerator(const std::string& description, ui::Accelerator* accelerator);

// Set platform accelerator for the Accelerator.
void SetPlatformAccelerator(ui::Accelerator* accelerator);

// Generate a table that contains memu model's accelerators and command ids.
void GenerateAcceleratorTable(AcceleratorTable* table, meson::MesonMenuModel* model);

// Trigger command from the accelerators table.
bool TriggerAcceleratorTableCommand(AcceleratorTable* table, const ui::Accelerator& accelerator);
}
