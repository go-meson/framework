#include "api/window_binding.h"
#include "api/web_contents_binding.h"
#include "browser/session/meson_browser_context.h"
#include "browser/browser_client.h"
#include "browser/native_window.h"
#include "common/options_switches.h"
#include "base/threading/thread_task_runner_handle.h"
#include "api/session_binding.h"
#include "api/web_contents_binding.h"
#include "api/api.h"

namespace meson {
template <>
const APIBindingT<WindowBinding, WindowClassBinding>::MethodTable APIBindingT<WindowBinding, WindowClassBinding>::methodTable = {
    {"loadURL", std::mem_fn(&WindowBinding::LoadURL)},
    {"close", std::mem_fn(&WindowBinding::Close)},
    {"webcontents", std::mem_fn(&WindowBinding::GetWebContents)},
    {"openDevTools", std::mem_fn(&WindowBinding::OpenDevTools)},
    {"closeDevTools", std::mem_fn(&WindowBinding::CloseDevTools)},
    {"isDevToolsOpened", std::mem_fn(&WindowBinding::IsDevToolsOpened)},
};

template <>
const APIClassBindingT<WindowBinding, WindowClassBinding>::MethodTable APIClassBindingT<WindowBinding, WindowClassBinding>::staticMethodTable = {
    {"_create", std::mem_fn(&WindowClassBinding::CreateInstance)},
};

MESON_IMPLEMENT_API_CLASS(WindowBinding, WindowClassBinding);

// TODO: インスタンス生成と初期化を分割するか、エラー時に例外をスローする
WindowBinding::WindowBinding(unsigned int id, const base::DictionaryValue& args)
    : APIBindingT(MESON_OBJECT_TYPE_WINDOW, id) {
  DLOG(INFO) << "Meson Window construct [" << this << "] " << id_;

  int web_contents_id = -1;
  if (args.GetInteger("web_contents_id", &web_contents_id)) {
    web_contents_ = WebContentsBinding::Class().GetBinding(web_contents_id);
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

    web_contents_ = static_cast<WebContentsClassBinding&>(WebContentsBinding::Class()).NewInstance(*newWebPreference);
  }

  // Keep a copy of the options for later use.
  //mate::Dictionary(isolate, web_contents->GetWrapper()).Set("browserWindowOptions", options);

  // The parent window.
  int parent_id = -1;
  if (args.GetInteger("parent", &parent_id)) {
    parent_window_ = WindowBinding::Class().GetBinding(parent_id);
  }

  // Creates BrowserWindow.
  window_.reset(NativeWindow::Create(
      web_contents_->managed_web_contents(),
      args,
      parent_window_ ? parent_window_->window_.get() : nullptr));
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
    parent_window_->child_windows_[id] = base::AsWeakPtr(this);
}
WindowBinding::~WindowBinding(void) {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  if (!window_->IsClosed()) {
    window_->CloseContents(nullptr);
  }
  // Destroy the native window in next tick because the native code might be
  // iterating all windows.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, window_.release());
}

void WindowBinding::WillCloseWindow(bool* prevent_default) {
  auto prevent = EmitPreventEvent("close");
  LOG(INFO) << "WillCloseWindow: " << prevent;
  *prevent_default = prevent;
}

void WindowBinding::WillDestroyNativeObject() {
  // Close all child windows before closing current window.
  LOG(INFO) << __PRETTY_FUNCTION__;
  for (auto& kv : child_windows_) {
    if (kv.second) {
      kv.second->window_->CloseImmediately();
    }
  }
}

#if 1
namespace {
void windowFinalizer(scoped_refptr<WindowBinding> self) {
  LOG(INFO) << __PRETTY_FUNCTION__ << (long)self.get();
}
}
#endif

void WindowBinding::OnWindowClosed() {
  LOG(INFO) << __PRETTY_FUNCTION__;
  web_contents_->DestroyWebContents();

  //RemoveFromWeakMap();
  window_->RemoveObserver(this);

  scoped_refptr<WindowBinding> self(this);

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  //MarkDestroyed();

  EmitEvent("closed");

  Class().RemoveBinding(this);

//RemoveFromParentChildWindows();

// Destroy the native class when window is closed.
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, base::Bind(windowFinalizer, self));
}

void WindowBinding::OnWindowBlur() {
  EmitEvent("blur");
}

void WindowBinding::OnWindowFocus() {
  EmitEvent("focus");
}

void WindowBinding::OnWindowShow() {
  EmitEvent("show");
}

void WindowBinding::OnWindowHide() {
  EmitEvent("hide");
}

void WindowBinding::OnReadyToShow() {
  EmitEvent("ready-to-show");
}

void WindowBinding::OnWindowMaximize() {
  EmitEvent("maximize");
}

void WindowBinding::OnWindowUnmaximize() {
  EmitEvent("unmaximize");
}

void WindowBinding::OnWindowMinimize() {
  EmitEvent("minimize");
}

void WindowBinding::OnWindowRestore() {
  EmitEvent("restore");
}

void WindowBinding::OnWindowResize() {
  EmitEvent("resize");
}

void WindowBinding::OnWindowMove() {
  EmitEvent("move");
}

void WindowBinding::OnWindowMoved() {
  EmitEvent("moved");
}

void WindowBinding::OnWindowEnterFullScreen() {
  EmitEvent("enter-full-screen");
}

void WindowBinding::OnWindowLeaveFullScreen() {
  EmitEvent("leave-full-screen");
}

void WindowBinding::OnWindowScrollTouchBegin() {
  EmitEvent("scroll-touch-begin");
}

void WindowBinding::OnWindowScrollTouchEnd() {
  EmitEvent("scroll-touch-end");
}

void WindowBinding::OnWindowScrollTouchEdge() {
  EmitEvent("scroll-touch-edge");
}

void WindowBinding::OnWindowSwipe(const std::string& direction) {
  //Emit("swipe", direction);
}

void WindowBinding::OnWindowEnterHtmlFullScreen() {
  EmitEvent("enter-html-full-screen");
}

void WindowBinding::OnWindowLeaveHtmlFullScreen() {
  EmitEvent("leave-html-full-screen");
}

void WindowBinding::OnRendererUnresponsive() {
  EmitEvent("unresponsive");
}

void WindowBinding::OnRendererResponsive() {
  EmitEvent("responsive");
}

void WindowBinding::OnExecuteWindowsCommand(const std::string& command_name) {
  //Emit("app-command", command_name);
}

#if defined(OS_WIN)
void WindowBinding::OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) {
  if (IsWindowMessageHooked(message)) {
    messages_callback_map_[message].Run(ToBuffer(isolate(), static_cast<void*>(&w_param), sizeof(WPARAM)),
                                        ToBuffer(isolate(), static_cast<void*>(&l_param), sizeof(LPARAM)));
  }
}
#endif

api::MethodResult WindowBinding::LoadURL(const api::APIArgs& args) {
  std::string url;
  const base::DictionaryValue* opt;
  base::DictionaryValue dummy;
  if (!args.GetString(0, &url)) {
    DCHECK(false);
    return api::MethodResult("invalid argument");
  }
  if (!args.GetDictionary(1, &opt)) {
    opt = &dummy;
  }
  web_contents_->LoadURL(GURL(url), *opt);
  return api::MethodResult();
}

api::MethodResult WindowBinding::Close(const api::APIArgs& args) {
  DCHECK(0 == args.GetSize());
  window_->Close();
  return api::MethodResult();
}

api::MethodResult WindowBinding::GetWebContents(const api::APIArgs& args) {
  DCHECK(0 == args.GetSize());
  CHECK(web_contents_);
  int contents_id = web_contents_->GetID();
  std::unique_ptr<base::Value> ret(new base::FundamentalValue(contents_id));
  // bind?
  return api::MethodResult(std::move(ret));
}

api::MethodResult WindowBinding::OpenDevTools(const api::APIArgs& args) {
  web_contents_->OpenDevTools(nullptr);
  return api::MethodResult();
}

api::MethodResult WindowBinding::CloseDevTools(const api::APIArgs& args) {
  web_contents_->CloseDevTools();
  return api::MethodResult();
}

api::MethodResult WindowBinding::IsDevToolsOpened(const api::APIArgs& args) {
  bool f = web_contents_->IsDevToolsOpened();
  std::unique_ptr<base::Value> ret(new base::FundamentalValue(f));
  return api::MethodResult(std::move(ret));
}

bool WindowBinding::IsFocused() const {
  return window_->IsFocused();
}

WindowClassBinding::WindowClassBinding(void)
    : APIClassBindingT(MESON_OBJECT_TYPE_WINDOW) {}

WindowClassBinding::~WindowClassBinding(void) {
}

api::MethodResult WindowClassBinding::CreateInstance(const api::APIArgs& args) {
  const base::DictionaryValue* opt = nullptr;
  if (!args.GetDictionary(0, &opt)) {
    return api::MethodResult("invalid argument.");
  }
  auto id = GetNextBindingID();
  scoped_refptr<WindowBinding> ret(new WindowBinding(id, *opt));
  SetBinding(id, ret);
  return api::MethodResult(ret);
}
}
