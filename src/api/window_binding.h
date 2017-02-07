//-*-c++-*-
#pragma once

#include <memory>
#include "base/memory/ref_counted.h"
#include "base/callback.h"
#include "api/api_binding.h"
#include "browser/meson_window.h"
#include "browser/native_window_observer.h"

namespace meson {
class NativeWindow;
class WindowClassBinding;
class WebContentsBinding;
class WindowBinding : public APIBindingT<WindowBinding, WindowClassBinding>, public NativeWindowObserver {
 public:
  enum { ObjType = MESON_OBJECT_TYPE_WINDOW };

 public:
  WindowBinding(unsigned int id, const base::DictionaryValue& args);
  virtual ~WindowBinding(void);

 public:  // Local Methods
  api::MethodResult LoadURL(const api::APIArgs& args);
  api::MethodResult Close(const api::APIArgs& args);
  api::MethodResult GetWebContents(const api::APIArgs& args);
  api::MethodResult OpenDevTools(const api::APIArgs& args);
  api::MethodResult CloseDevTools(const api::APIArgs& args);
  api::MethodResult IsDevToolsOpened(const api::APIArgs& args);

 public:  // static metods
 public:  // NativeWindowObserver:
  void WillCloseWindow(bool* prevent_default) override;
  void WillDestroyNativeObject() override;
  void OnWindowClosed() override;
  void OnWindowBlur() override;
  void OnWindowFocus() override;
  void OnWindowShow() override;
  void OnWindowHide() override;
  void OnReadyToShow() override;
  void OnWindowMaximize() override;
  void OnWindowUnmaximize() override;
  void OnWindowMinimize() override;
  void OnWindowRestore() override;
  void OnWindowResize() override;
  void OnWindowMove() override;
  void OnWindowMoved() override;
  void OnWindowScrollTouchBegin() override;
  void OnWindowScrollTouchEnd() override;
  void OnWindowScrollTouchEdge() override;
  void OnWindowSwipe(const std::string& direction) override;
  void OnWindowEnterFullScreen() override;
  void OnWindowLeaveFullScreen() override;
  void OnWindowEnterHtmlFullScreen() override;
  void OnWindowLeaveHtmlFullScreen() override;
  void OnRendererUnresponsive() override;
  void OnRendererResponsive() override;
  void OnExecuteWindowsCommand(const std::string& command_name) override;

#if defined(OS_WIN)
  void OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) override;
#endif
 public:
  NativeWindow* window() const { return window_.get(); }
  bool IsFocused() const;

 protected:
  std::unique_ptr<NativeWindow> window_;
  scoped_refptr<WebContentsBinding> web_contents_;
  scoped_refptr<WindowBinding> parent_window_;
  std::map<unsigned int, base::WeakPtr<WindowBinding>> child_windows_;
};

class WindowClassBinding : public APIClassBindingT<WindowBinding, WindowClassBinding> {
 public:
  WindowClassBinding(void);
  ~WindowClassBinding(void) override;

 public:
  api::MethodResult CreateInstance(const api::APIArgs& args);
};
}
