#include "browser/ui/message_box.h"
#include "ui/gfx/image/image_skia.h"
#include "api/window_binding.h"
#include "api/meson.h"
#include "api/dialog_binding.h"

namespace meson {
template <>
const APIBindingT<DialogClassBinding, DialogClassBinding>::MethodTable APIBindingT<DialogClassBinding, DialogClassBinding>::methodTable = {};

template <>
const APIClassBindingT<DialogClassBinding, DialogClassBinding>::MethodTable APIClassBindingT<DialogClassBinding, DialogClassBinding>::staticMethodTable = {
    {"showMessageBox", std::mem_fn(&DialogClassBinding::ShowMessageBox)},
};

MESON_IMPLEMENT_API_CLASS(DialogClassBinding, DialogClassBinding);

DialogClassBinding::DialogClassBinding(void)
    : APIClassBindingT<meson::DialogClassBinding, meson::DialogClassBinding>(MESON_OBJECT_TYPE_DIALOG) {
}
DialogClassBinding::~DialogClassBinding(void) {
}

void DialogClassBinding::ShowMessageBoxCallback(const std::string& eventName, int buttonId) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << " : " << eventName << ", " << buttonId;
  EmitEvent(eventName, "buttonID", buttonId);
}

api::MethodResult DialogClassBinding::ShowMessageBox(const api::APIArgs& args) {
  int window_id = -1;
  const base::DictionaryValue* opt = nullptr;
  std::string callback_event_name;
  if (!args.GetInteger(0, &window_id) || !args.GetDictionary(1, &opt)) {
    return api::MethodResult("invalid argument");
  }
  args.GetString(2, &callback_event_name);
  NativeWindow* parent_window = nullptr;

  if (window_id != 0) {
    auto win = WindowBinding::Class().GetBinding(window_id);
    if (!win) {
      return api::MethodResult("invalid parent window");
    }
    parent_window = win->window();
  }

  int type = MESSAGE_BOX_TYPE_NONE;
  int default_id = -1;
  int cancel_id = -1;
  int options = 0;
  const base::ListValue* p;
  std::string title, message, detail;
  //const gfx::ImageSkia& icon,
  //atom::NativeWindow* window,
  opt->GetInteger("type", &type);
  std::vector<std::string> buttons;
  if (opt->GetList("buttons", &p)) {
    buttons.resize(p->GetSize());
    for (size_t idx = 0; idx < p->GetSize(); idx++) {
      p->GetString(idx, &buttons[idx]);
    }
  }
  opt->GetString("title", &title);
  opt->GetString("message", &message);
  opt->GetString("detail", &detail);
  opt->GetInteger("defaultId", &default_id);
  opt->GetInteger("cancelId", &cancel_id);
  opt->GetInteger("option", &options);

  gfx::ImageSkia icon;
  if (callback_event_name.empty()) {
    int ret = ::meson::ShowMessageBox(parent_window, static_cast<MessageBoxType>(type), buttons, default_id, cancel_id, options, title, message, detail, icon);
    return api::MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
  } else {
    MessageBoxCallback callback = base::Bind(&DialogClassBinding::ShowMessageBoxCallback, this, callback_event_name);
    ::meson::ShowMessageBox(parent_window, static_cast<MessageBoxType>(type), buttons, default_id, cancel_id, options, title, message, detail, icon, callback);
    return api::MethodResult();
  }
}
}
