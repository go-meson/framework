//-*-c++-*-
#pragma once

#include <string>

#include "base/strings/string16.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace meson {
class NativeWindowObserver {
 public:
  virtual ~NativeWindowObserver() {}
public:
  virtual void WillCreatePopupWindow(const base::string16& frame_name,
                                     const GURL& target_url,
                                     const std::string& partition_id,
                                     WindowOpenDisposition disposition) {}
  virtual void WillNavigate(bool* prevent_default, const GURL& url) {}
  virtual void WillCloseWindow(bool* prevent_default) {}
  virtual void WillDestroyNativeObject() {}
  virtual void OnWindowClosed() {}
  virtual void OnWindowBlur() {}
  virtual void OnWindowFocus() {}
  virtual void OnWindowShow() {}
  virtual void OnWindowHide() {}
  virtual void OnReadyToShow() {}
  virtual void OnWindowMaximize() {}
  virtual void OnWindowUnmaximize() {}
  virtual void OnWindowMinimize() {}
  virtual void OnWindowRestore() {}
  virtual void OnWindowResize() {}
  virtual void OnWindowMove() {}
  virtual void OnWindowMoved() {}
  virtual void OnWindowScrollTouchBegin() {}
  virtual void OnWindowScrollTouchEnd() {}
  virtual void OnWindowScrollTouchEdge() {}
  virtual void OnWindowSwipe(const std::string& direction) {}
  virtual void OnWindowEnterFullScreen() {}
  virtual void OnWindowLeaveFullScreen() {}
  virtual void OnWindowEnterHtmlFullScreen() {}
  virtual void OnWindowLeaveHtmlFullScreen() {}
#if defined(OS_WIN)
  virtual void OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) {}
#endif
  virtual void OnRendererUnresponsive() {}
  virtual void OnRendererResponsive() {}
  virtual void OnExecuteWindowsCommand(const std::string& command_name) {}
};

}  // namespace atom
