#include "api/window_binding.h"
#include "api/web_contents_binding.h"
#include "browser/session/meson_browser_context.h"
#include "browser/browser_client.h"
#include "browser/native_window.h"
#include "common/options_switches.h"
#include "api/session_binding.h"
#include "api/web_contents_binding.h"
#include "api/api.h"

namespace meson {
template <>
APIBindingT<MesonWindowBinding>::MethodTable APIBindingT<MesonWindowBinding>::methodTable = {
    {"loadURL", std::mem_fn(&MesonWindowBinding::LoadURL)},
    {"close", std::mem_fn(&MesonWindowBinding::Close)},
};
MesonWindowBinding::MesonWindowBinding(unsigned int id, const api::APICreateArg& args)
    : APIBindingT<MesonWindowBinding>(MESON_OBJECT_TYPE_WINDOW, id) {
  DLOG(INFO) << "Meson Window construct [" << this << "] " << id_;

  int web_contents_id = -1;
  if (args.GetInteger("web_contents_id", &web_contents_id)) {
    web_contents_ = static_cast<MesonWebContentsBinding*>(API::Get()->GetBinding(web_contents_id).get());
  } else {
    std::unique_ptr<base::DictionaryValue> newWebPreference;
    const base::DictionaryValue* pPreference;
    if (args.GetDictionary(options::kWebPreferences, &pPreference)) {
      newWebPreference.reset(pPreference->DeepCopy());
    } else {
      newWebPreference.reset(new base::DictionaryValue());
    }

    std::string backgroundColor;
    if (args.GetString(options::kBackgroundColor, &backgroundColor)) {
      newWebPreference->SetString(switches::kBackgroundColor, backgroundColor);
    }
    bool transparent = false;
    if (args.GetBoolean(options::kTransparent, &transparent)) {
      newWebPreference->SetBoolean("transparent", transparent);
    }
    web_contents_ = API::Get()->Create<MesonWebContentsBinding>(MESON_OBJECT_TYPE_WEB_CONTENTS, *newWebPreference);
  }

  // Keep a copy of the options for later use.
  //mate::Dictionary(isolate, web_contents->GetWrapper()).Set("browserWindowOptions", options);

  // The parent window.
  int parent_id = -1;
  if (args.GetInteger("parent", &parent_id)) {
    parent_window_ = static_cast<MesonWindowBinding*>(API::Get()->GetBinding(parent_id).get());
  }

  // Creates BrowserWindow.
  window_.reset(NativeWindow::Create(web_contents_->managed_web_contents(), args, parent_window_ ? parent_window_->window_.get() : nullptr));
  web_contents_->SetOwnerWindow(window_.get());

#if defined(TOOLKIT_VIEWS)
  // Sets the window icon.
  mate::Handle<NativeImage> icon;
  if (options.Get(options::kIcon, &icon))
    SetIcon(icon);
#endif

  window_->InitFromOptions(args);
  window_->AddObserver(this);

  if (parent_window_)
    parent_window_->child_windows_[id] = GetWeakPtr();
}
MesonWindowBinding::~MesonWindowBinding(void) {
  DLOG(INFO) << __PRETTY_FUNCTION__;
}

void MesonWindowBinding::WillCloseWindow(bool* prevent_default) {
  //TODO: 返却値をどうやって受け取ろう……。
  auto prevent = EmitPreventEvent("close");
  LOG(INFO) << "WillCloseWindow: " << prevent;
  *prevent_default = prevent;
}

void MesonWindowBinding::WillDestroyNativeObject() {
  // Close all child windows before closing current window.
  DLOG(INFO) << __PRETTY_FUNCTION__;
  for (auto& kv : child_windows_) {
    if (kv.second) {
      kv.second->window_->CloseImmediately();
    }
  }
}

void MesonWindowBinding::OnWindowClosed() {
  web_contents_->DestroyWebContents();

  //RemoveFromWeakMap();
  window_->RemoveObserver(this);

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  //MarkDestroyed();

  EmitEvent("closed");

  //RemoveFromParentChildWindows();

  // Destroy the native class when window is closed.
  //base::MessageLoop::current()->PostTask(FROM_HERE, GetDestroyClosure());
}

void MesonWindowBinding::OnWindowBlur() {
  EmitEvent("blur");
}

void MesonWindowBinding::OnWindowFocus() {
  EmitEvent("focus");
}

void MesonWindowBinding::OnWindowShow() {
  EmitEvent("show");
}

void MesonWindowBinding::OnWindowHide() {
  EmitEvent("hide");
}

void MesonWindowBinding::OnReadyToShow() {
  EmitEvent("ready-to-show");
}

void MesonWindowBinding::OnWindowMaximize() {
  EmitEvent("maximize");
}

void MesonWindowBinding::OnWindowUnmaximize() {
  EmitEvent("unmaximize");
}

void MesonWindowBinding::OnWindowMinimize() {
  EmitEvent("minimize");
}

void MesonWindowBinding::OnWindowRestore() {
  EmitEvent("restore");
}

void MesonWindowBinding::OnWindowResize() {
  EmitEvent("resize");
}

void MesonWindowBinding::OnWindowMove() {
  EmitEvent("move");
}

void MesonWindowBinding::OnWindowMoved() {
  EmitEvent("moved");
}

void MesonWindowBinding::OnWindowEnterFullScreen() {
  EmitEvent("enter-full-screen");
}

void MesonWindowBinding::OnWindowLeaveFullScreen() {
  EmitEvent("leave-full-screen");
}

void MesonWindowBinding::OnWindowScrollTouchBegin() {
  EmitEvent("scroll-touch-begin");
}

void MesonWindowBinding::OnWindowScrollTouchEnd() {
  EmitEvent("scroll-touch-end");
}

void MesonWindowBinding::OnWindowScrollTouchEdge() {
  EmitEvent("scroll-touch-edge");
}

void MesonWindowBinding::OnWindowSwipe(const std::string& direction) {
  //Emit("swipe", direction);
}

void MesonWindowBinding::OnWindowEnterHtmlFullScreen() {
  EmitEvent("enter-html-full-screen");
}

void MesonWindowBinding::OnWindowLeaveHtmlFullScreen() {
  EmitEvent("leave-html-full-screen");
}

void MesonWindowBinding::OnRendererUnresponsive() {
  EmitEvent("unresponsive");
}

void MesonWindowBinding::OnRendererResponsive() {
  EmitEvent("responsive");
}

void MesonWindowBinding::OnExecuteWindowsCommand(const std::string& command_name) {
  //Emit("app-command", command_name);
}

#if defined(OS_WIN)
void MesonWindowBinding::OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) {
  if (IsWindowMessageHooked(message)) {
    messages_callback_map_[message].Run(ToBuffer(isolate(), static_cast<void*>(&w_param), sizeof(WPARAM)),
                                        ToBuffer(isolate(), static_cast<void*>(&l_param), sizeof(LPARAM)));
  }
}
#endif

MesonWindowBinding::MethodResult MesonWindowBinding::LoadURL(const api::APIArgs& args) {
  std::string url;
  const base::DictionaryValue* opt;
  base::DictionaryValue dummy;
  if (!args.GetString(0, &url)) {
    DCHECK(false);
    return MethodResult("invalid argument");
  }
  if (!args.GetDictionary(1, &opt)) {
    opt = &dummy;
  }
  web_contents_->LoadURL(GURL(url), *opt);
  return MethodResult();
}

MesonWindowBinding::MethodResult MesonWindowBinding::Close(const api::APIArgs& args) {
  DCHECK(0 == args.GetSize());
  window_->Close();
  return MethodResult();
}

MesonWindowFactory::MesonWindowFactory() {}
MesonWindowFactory::~MesonWindowFactory() {}
APIBinding* MesonWindowFactory::Create(unsigned int id, const api::APICreateArg& args) {
  return new MesonWindowBinding(id, args);
}
}
