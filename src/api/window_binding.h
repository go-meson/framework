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
class MesonWebContentsBinding;
class MesonWindowBinding : public APIBindingT<MesonWindowBinding>, public NativeWindowObserver {
 public:
  MesonWindowBinding(unsigned int id, const api::APICreateArg& args);
  virtual ~MesonWindowBinding(void);

 public:  // Local Methods
  MethodResult LoadURL(const api::APIArgs& args);

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

 protected:
  std::unique_ptr<NativeWindow> window_;
  scoped_refptr<MesonWebContentsBinding> web_contents_;
  scoped_refptr<MesonWindowBinding> parent_window_;
  std::map<unsigned int, base::WeakPtr<MesonWindowBinding>> child_windows_;
};

class MesonWindowFactory : public APIBindingFactory {
 public:
  MesonWindowFactory(void);
  virtual ~MesonWindowFactory(void);

 public:
  virtual APIBinding* Create(unsigned int id, const api::APICreateArg& args) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MesonWindowFactory);
};
}
