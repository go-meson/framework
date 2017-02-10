#include "browser/ui/meson_menu_model.h"

#include "base/stl_util.h"

namespace meson {
MesonMenuModel::MesonMenuModel(Delegate* delegate)
    : ui::SimpleMenuModel(delegate), delegate_(delegate) {
}

MesonMenuModel::~MesonMenuModel() {
}

void MesonMenuModel::SetRole(int index, const base::string16& role) {
  int command_id = GetCommandIdAt(index);
  roles_[command_id] = role;
}

base::string16 MesonMenuModel::GetRoleAt(int index) {
  int command_id = GetCommandIdAt(index);
  if (ContainsKey(roles_, command_id))
    return roles_[command_id];
  else
    return base::string16();
}

bool MesonMenuModel::GetAcceleratorAtWithParams(int index, bool use_default_accelerator, ui::Accelerator* accelerator) const {
  if (delegate_) {
    return delegate_->GetAcceleratorForCommandIdWithParams(GetCommandIdAt(index), use_default_accelerator, accelerator);
  }
  return false;
}

void MesonMenuModel::MenuWillClose() {
  ui::SimpleMenuModel::MenuWillClose();
  for (Observer& observer : observers_)
    observer.MenuWillClose();
}

MesonMenuModel* MesonMenuModel::GetSubmenuModelAt(int index) {
  return static_cast<MesonMenuModel*>(ui::SimpleMenuModel::GetSubmenuModelAt(index));
}
}
