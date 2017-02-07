#include "browser/browser_main_parts.h"
#include "api/api_server.h"
#include "api/api.h"
#include "api/app_binding.h"
#include "api/dialog_binding.h"
#include "api/menu_binding.h"
#include "api/session_binding.h"
#include "api/window_binding.h"
#include "api/web_contents_binding.h"
#include "browser/browser.h"

namespace meson {
MesonMainParts* MesonMainParts::self_ = nullptr;

MesonMainParts::MesonMainParts()
    : browser_(new Browser),
      api_(new API()),
      exit_code_(nullptr) {
  DCHECK(!self_) << "MesonMainParts already created!";
  self_ = this;
}

MesonMainParts::~MesonMainParts() {}

void MesonMainParts::PreMainMessageLoopRun(void) {
  brightray::BrowserMainParts::PreMainMessageLoopRun();

  api_->InstallBindings(MESON_OBJECT_TYPE_APP, new AppClassBinding());
  api_->InstallBindings(MESON_OBJECT_TYPE_DIALOG, new DialogClassBinding());
  api_->InstallBindings(MESON_OBJECT_TYPE_SESSION, new SessionClassBinding());
  api_->InstallBindings(MESON_OBJECT_TYPE_WINDOW, new WindowClassBinding());
  api_->InstallBindings(MESON_OBJECT_TYPE_WEB_CONTENTS, new WebContentsClassBinding());
  api_->InstallBindings(MESON_OBJECT_TYPE_MENU, new MenuClassBinding());
  api_server_ = new APIServer(*api_);
  api_server_->Start();
}
void MesonMainParts::PostMainMessageLoopRun() {
  LOG(INFO) << __PRETTY_FUNCTION__;
  brightray::BrowserMainParts::PostMainMessageLoopRun();

#if defined(OS_MACOSX)
  FreeAppDelegate();
#endif
}

bool MesonMainParts::MainMessageLoopRun(int* result_code) {
  exit_code_ = result_code;
  return brightray::BrowserMainParts::MainMessageLoopRun(result_code);
}

MesonMainParts* MesonMainParts::Get(void) {
  DCHECK(self_);
  return self_;
}
bool MesonMainParts::SetExitCode(int exit_code) {
  if (!exit_code_) {
    return false;
  }
  *exit_code_ = exit_code;
  return true;
}
int MesonMainParts::GetExitCode() const {
  return exit_code_ ? *exit_code_ : 0;
}
}
