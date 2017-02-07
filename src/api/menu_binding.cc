#include "api/menu_binding.h"
#include "browser/native_window.h"
#include "browser/ui/accelerator_util.h"
#include "api/api.h"

#include "api/window_binding.h"

namespace meson {
template <>
const APIBindingT<MenuBinding, MenuClassBinding>::MethodTable APIBindingT<MenuBinding, MenuClassBinding>::methodTable = {
  {"loadTemplate", std::mem_fn(&MenuBinding::LoadTemplate)},
  {"insertItem", std::mem_fn(&MenuBinding::InsertItemAt)},
};
template <>
const APIClassBindingT<MenuBinding, MenuClassBinding>::MethodTable APIClassBindingT<MenuBinding, MenuClassBinding>::staticMethodTable = {
  {"_create", std::mem_fn(&MenuClassBinding::CreateInstance)},
  {"setApplicationMenu", std::mem_fn(&MenuClassBinding::SetApplicationMenu)},
};

MESON_IMPLEMENT_API_CLASS(MenuBinding, MenuClassBinding);

MenuBinding::MenuBinding(api::ObjID id)
    : APIBindingT<MenuBinding, MenuClassBinding>(MESON_OBJECT_TYPE_MENU, id),
      model_(new MesonMenuModel(this)),
      parent_() {}

MenuBinding::~MenuBinding() {}

bool MenuBinding::IsCommandIdChecked(int command_id) const {
  //return is_checked_.Run(command_id);
  return false;
}

bool MenuBinding::IsCommandIdEnabled(int command_id) const {
  //return is_enabled_.Run(command_id);
  return true;
}

bool MenuBinding::IsCommandIdVisible(int command_id) const {
  return true;
}

bool MenuBinding::GetAcceleratorForCommandIdWithParams(int command_id, bool use_default_accelerator, ui::Accelerator* accelerator) const {
  auto fiter = accelerators_.find(command_id);
  if (fiter != accelerators_.end()) {
    LOG(INFO) << "find_accelerator : " << command_id;
    *accelerator = (*fiter).second;
    return true;
  }
  return false;
}

void MenuBinding::ExecuteCommand(int command_id, int flags) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << command_id << ", " << flags << ")";
  auto focusWindow = WindowBinding::Class().FindBinding([](const WindowBinding& w) {
    return w.IsFocused();
  });
  if (!focusWindow)
    return;
  int focusId = focusWindow->GetID();
  auto fiter = clickEvents_.find(command_id);
  if (fiter != clickEvents_.end()) {
    EmitEvent((*fiter).second, "focusID", focusId);
  }
}

void MenuBinding::MenuWillShow(ui::SimpleMenuModel* source) {
  LOG(INFO) << __PRETTY_FUNCTION__;
#if 0
  menu_will_show_.Run();
#endif
}

api::MethodResult MenuBinding::LoadTemplate(const api::APIArgs& args) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  for (size_t idx = 0; idx < args.GetSize(); idx++) {
    const base::DictionaryValue* item;
    if (!args.GetDictionary(idx, &item)) {
      return api::MethodResult("invalid argument(not dictionary)");
    }
    int type = MESON_MENU_TYPE_NORMAL;
    int command_id = 0;
    item->GetInteger("type", &type);
    item->GetInteger("id", &command_id);
    base::string16 label;
    item->GetString("label", &label);
    if (type == MESON_MENU_TYPE_SUBMENU) {
      int subMenuID = -1;
      item->GetInteger("subMenuId", &subMenuID);
      DCHECK(subMenuID > 0);
      auto submenu = Class().GetBinding(subMenuID);
      DCHECK(submenu);
      submenu->parent_ = base::AsWeakPtr(this);
      model_->InsertSubMenuAt(idx, command_id, label, submenu->model());
    } else {
      //
      model_->InsertItemAt(idx, command_id, label);
    }
    base::string16 tmp;
    if (item->GetString("sublabel", &tmp)) {
      model_->SetMinorText(idx, tmp);
    }
#if 0
    //TODO
    if (item.icon != null) {
      this.setIcon(pos, item.icon)
        }
#endif
    if (item->GetString("role", &tmp)) {
      model_->SetRole(idx, tmp);
    }
    std::string accel;
    if (item->GetString("accelerator", &accel)) {
      accelerator_util::StringToAccelerator(accel, &accelerators_[command_id]);
    }
    std::string clickEventName;
    if (item->GetString("clickEventName", &clickEventName)) {
      clickEvents_[command_id] = clickEventName;
    }
  }
  return api::MethodResult();
}

api::MethodResult MenuBinding::InsertItemAt(const api::APIArgs& args) {
  int index;
  int command_id;
  base::string16 label;
  args.GetInteger(0, &index);
  args.GetInteger(1, &command_id);
  args.GetString(2, &label);
  model_->InsertItemAt(index, command_id, label);
  return api::MethodResult();
}

api::MethodResult MenuBinding::InsertSeparatorAt(const api::APIArgs& args) {
  int index;
  args.GetInteger(0, &index);
  model_->InsertSeparatorAt(index, ui::NORMAL_SEPARATOR);
  return api::MethodResult();
}

api::MethodResult MenuBinding::InsertCheckItemAt(const api::APIArgs& args) {
  int index;
  int command_id;
  base::string16 label;
  args.GetInteger(0, &index);
  args.GetInteger(1, &command_id);
  args.GetString(2, &label);
  model_->InsertCheckItemAt(index, command_id, label);
  return api::MethodResult();
}

api::MethodResult MenuBinding::InsertRadioItemAt(const api::APIArgs& args) {
  int index;
  int command_id;
  base::string16 label;
  int group_id;
  args.GetInteger(0, &index);
  args.GetInteger(1, &command_id);
  args.GetString(2, &label);
  args.GetInteger(3, &group_id);
  model_->InsertRadioItemAt(index, command_id, label, group_id);
  return api::MethodResult();
}

api::MethodResult MenuBinding::InsertSubMenuAt(const api::APIArgs& args) {
  int index;
  int command_id;
  base::string16 label;
  int menu_id;
  args.GetInteger(0, &index);
  args.GetInteger(1, &command_id);
  args.GetString(2, &label);
  args.GetInteger(3, &menu_id);
  auto menu = Class().GetBinding(menu_id);
  menu->parent_ = base::AsWeakPtr(this);
  model_->InsertSubMenuAt(index, command_id, label, menu->model_.get());
  return api::MethodResult();
}

#if 0
void MenuBinding::SetIcon(int index, const gfx::Image& image) {
  model_->SetIcon(index, image);
}
#endif

api::MethodResult MenuBinding::SetSublabel(const api::APIArgs& args) {
  int index;
  base::string16 sublabel;
  args.GetInteger(0, &index);
  args.GetString(1, &sublabel);
  model_->SetSublabel(index, sublabel);
  return api::MethodResult();
}

api::MethodResult MenuBinding::SetRole(const api::APIArgs& args) {
  int index;
  base::string16 role;
  args.GetInteger(0, &index);
  args.GetString(1, &role);
  model_->SetRole(index, role);
  return api::MethodResult();
}

api::MethodResult MenuBinding::Clear() {
  model_->Clear();
  return api::MethodResult();
}

api::MethodResult MenuBinding::GetIndexOfCommandId(const api::APIArgs& args) {
  int command_id;
  args.GetInteger(0, &command_id);
  int ret = model_->GetIndexOfCommandId(command_id);
  return api::MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

api::MethodResult MenuBinding::GetItemCount() const {
  int ret = model_->GetItemCount();
  return api::MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

api::MethodResult MenuBinding::GetCommandIdAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  int ret = model_->GetCommandIdAt(index);
  return api::MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

api::MethodResult MenuBinding::GetLabelAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  base::string16 ret = model_->GetLabelAt(index);
  return api::MethodResult(std::unique_ptr<base::Value>(new base::StringValue(ret)));
}

api::MethodResult MenuBinding::GetSublabelAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  base::string16 ret = model_->GetSublabelAt(index);
  return api::MethodResult(std::unique_ptr<base::Value>(new base::StringValue(ret)));
}

api::MethodResult MenuBinding::IsItemCheckedAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  auto ret = model_->IsItemCheckedAt(index);
  return api::MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

api::MethodResult MenuBinding::IsEnabledAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  auto ret = model_->IsEnabledAt(index);
  return api::MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

api::MethodResult MenuBinding::IsVisibleAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  auto ret = model_->IsVisibleAt(index);
  return api::MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

MenuClassBinding::MenuClassBinding(void)
    : APIClassBindingT<MenuBinding, MenuClassBinding>(MESON_OBJECT_TYPE_MENU) {
}
MenuClassBinding::~MenuClassBinding() {}
}
