#include "api/menu_binding.h"
#include "browser/native_window.h"
#include "browser/ui/accelerator_util.h"
#include "api/api.h"

namespace meson {
template <>
APIBindingT<MesonMenuBinding>::MethodTable APIBindingT<MesonMenuBinding>::methodTable = {
    {"loadTemplate", std::mem_fn(&MesonMenuBinding::LoadTemplate)},
    {"insertItem", std::mem_fn(&MesonMenuBinding::InsertItemAt)},
};

MesonMenuBinding::MesonMenuBinding(unsigned int id, const api::APICreateArg& args)
    : APIBindingT<meson::MesonMenuBinding>(MESON_OBJECT_TYPE_MENU, id),
      model_(new MesonMenuModel(this)),
      parent_() {
}

MesonMenuBinding::~MesonMenuBinding() {}

bool MesonMenuBinding::IsCommandIdChecked(int command_id) const {
  //return is_checked_.Run(command_id);
  return false;
}

bool MesonMenuBinding::IsCommandIdEnabled(int command_id) const {
  //return is_enabled_.Run(command_id);
  return true;
}

bool MesonMenuBinding::IsCommandIdVisible(int command_id) const {
  return true;
}

bool MesonMenuBinding::GetAcceleratorForCommandIdWithParams(int command_id, bool use_default_accelerator, ui::Accelerator* accelerator) const {
  auto fiter = accelerators_.find(command_id);
  if (fiter != accelerators_.end()) {
    LOG(INFO) << "find_accelerator : " << command_id;
    *accelerator = (*fiter).second;
    return true;
  }
  return false;
}

void MesonMenuBinding::ExecuteCommand(int command_id, int flags) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << command_id << ", " << flags << ")";
#if 0
  execute_command_.Run(mate::internal::CreateEventFromFlags(isolate(), flags), command_id);
#endif
  auto fiter = clickEvents_.find(command_id);
  if (fiter != clickEvents_.end()) {
    EmitEvent((*fiter).second);
  }
}

void MesonMenuBinding::MenuWillShow(ui::SimpleMenuModel* source) {
  LOG(INFO) << __PRETTY_FUNCTION__;
#if 0
  menu_will_show_.Run();
#endif
}

MesonMenuBinding::MethodResult MesonMenuBinding::LoadTemplate(const api::APIArgs& args) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  for (size_t idx = 0; idx < args.GetSize(); idx++) {
    const base::DictionaryValue* item;
    if (!args.GetDictionary(idx, &item)) {
      return MethodResult("invalid argument(not dictionary)");
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
      auto submenu = API::Get()->GetBinding<MesonMenuBinding>(subMenuID);
      DCHECK(submenu);
      submenu->parent_ = this->GetWeakPtr();
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
  return MethodResult();
}

MesonMenuBinding::MethodResult MesonMenuBinding::InsertItemAt(const api::APIArgs& args) {
  int index;
  int command_id;
  base::string16 label;
  args.GetInteger(0, &index);
  args.GetInteger(1, &command_id);
  args.GetString(2, &label);
  model_->InsertItemAt(index, command_id, label);
  return MethodResult();
}

MesonMenuBinding::MethodResult MesonMenuBinding::InsertSeparatorAt(const api::APIArgs& args) {
  int index;
  args.GetInteger(0, &index);
  model_->InsertSeparatorAt(index, ui::NORMAL_SEPARATOR);
  return MethodResult();
}

MesonMenuBinding::MethodResult MesonMenuBinding::InsertCheckItemAt(const api::APIArgs& args) {
  int index;
  int command_id;
  base::string16 label;
  args.GetInteger(0, &index);
  args.GetInteger(1, &command_id);
  args.GetString(2, &label);
  model_->InsertCheckItemAt(index, command_id, label);
  return MethodResult();
}

MesonMenuBinding::MethodResult MesonMenuBinding::InsertRadioItemAt(const api::APIArgs& args) {
  int index;
  int command_id;
  base::string16 label;
  int group_id;
  args.GetInteger(0, &index);
  args.GetInteger(1, &command_id);
  args.GetString(2, &label);
  args.GetInteger(3, &group_id);
  model_->InsertRadioItemAt(index, command_id, label, group_id);
  return MethodResult();
}

MesonMenuBinding::MethodResult MesonMenuBinding::InsertSubMenuAt(const api::APIArgs& args) {
  int index;
  int command_id;
  base::string16 label;
  int menu_id;
  args.GetInteger(0, &index);
  args.GetInteger(1, &command_id);
  args.GetString(2, &label);
  args.GetInteger(3, &menu_id);
  scoped_refptr<MesonMenuBinding> menu = static_cast<MesonMenuBinding*>(API::Get()->GetBinding(menu_id).get());
  menu->parent_ = this->GetWeakPtr();
  model_->InsertSubMenuAt(index, command_id, label, menu->model_.get());
  return MethodResult();
}

#if 0
void MesonMenuBinding::SetIcon(int index, const gfx::Image& image) {
  model_->SetIcon(index, image);
}
#endif

MesonMenuBinding::MethodResult MesonMenuBinding::SetSublabel(const api::APIArgs& args) {
  int index;
  base::string16 sublabel;
  args.GetInteger(0, &index);
  args.GetString(1, &sublabel);
  model_->SetSublabel(index, sublabel);
  return MethodResult();
}

MesonMenuBinding::MethodResult MesonMenuBinding::SetRole(const api::APIArgs& args) {
  int index;
  base::string16 role;
  args.GetInteger(0, &index);
  args.GetString(1, &role);
  model_->SetRole(index, role);
  return MethodResult();
}

MesonMenuBinding::MethodResult MesonMenuBinding::Clear() {
  model_->Clear();
  return MethodResult();
}

MesonMenuBinding::MethodResult MesonMenuBinding::GetIndexOfCommandId(const api::APIArgs& args) {
  int command_id;
  args.GetInteger(0, &command_id);
  int ret = model_->GetIndexOfCommandId(command_id);
  return MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

MesonMenuBinding::MethodResult MesonMenuBinding::GetItemCount() const {
  int ret = model_->GetItemCount();
  return MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

MesonMenuBinding::MethodResult MesonMenuBinding::GetCommandIdAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  int ret = model_->GetCommandIdAt(index);
  return MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

MesonMenuBinding::MethodResult MesonMenuBinding::GetLabelAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  base::string16 ret = model_->GetLabelAt(index);
  return MethodResult(std::unique_ptr<base::Value>(new base::StringValue(ret)));
}

MesonMenuBinding::MethodResult MesonMenuBinding::GetSublabelAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  base::string16 ret = model_->GetSublabelAt(index);
  return MethodResult(std::unique_ptr<base::Value>(new base::StringValue(ret)));
}

MesonMenuBinding::MethodResult MesonMenuBinding::IsItemCheckedAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  auto ret = model_->IsItemCheckedAt(index);
  return MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

MesonMenuBinding::MethodResult MesonMenuBinding::IsEnabledAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  auto ret = model_->IsEnabledAt(index);
  return MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

MesonMenuBinding::MethodResult MesonMenuBinding::IsVisibleAt(const api::APIArgs& args) const {
  int index;
  args.GetInteger(0, &index);
  auto ret = model_->IsVisibleAt(index);
  return MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
}

#if 0
// static
void Menu::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Menu"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("insertItem", &Menu::InsertItemAt)
      .SetMethod("insertCheckItem", &Menu::InsertCheckItemAt)
      .SetMethod("insertRadioItem", &Menu::InsertRadioItemAt)
      .SetMethod("insertSeparator", &Menu::InsertSeparatorAt)
      .SetMethod("insertSubMenu", &Menu::InsertSubMenuAt)
      .SetMethod("setIcon", &Menu::SetIcon)
      .SetMethod("setSublabel", &Menu::SetSublabel)
      .SetMethod("setRole", &Menu::SetRole)
      .SetMethod("clear", &Menu::Clear)
      .SetMethod("getIndexOfCommandId", &Menu::GetIndexOfCommandId)
      .SetMethod("getItemCount", &Menu::GetItemCount)
      .SetMethod("getCommandIdAt", &Menu::GetCommandIdAt)
      .SetMethod("getLabelAt", &Menu::GetLabelAt)
      .SetMethod("getSublabelAt", &Menu::GetSublabelAt)
      .SetMethod("isItemCheckedAt", &Menu::IsItemCheckedAt)
      .SetMethod("isEnabledAt", &Menu::IsEnabledAt)
      .SetMethod("isVisibleAt", &Menu::IsVisibleAt)
      .SetMethod("popupAt", &Menu::PopupAt);
}
#endif

MesonMenuFactory::MesonMenuFactory() {}
MesonMenuFactory::~MesonMenuFactory() {}
}

#if 0
namespace {

using atom::api::Menu;

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused, v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  Menu::SetConstructor(isolate, base::Bind(&Menu::New));

  mate::Dictionary dict(isolate, exports);
  dict.Set("Menu", Menu::GetConstructor(isolate)->GetFunction());
#if defined(OS_MACOSX)
  dict.SetMethod("setApplicationMenu", &Menu::SetApplicationMenu);
  dict.SetMethod("sendActionToFirstResponder",
                 &Menu::SendActionToFirstResponder);
#endif
}


}  // namespace
#endif
