#pragma once

#include <memory>

#include "brightray/common/main_delegate.h"
namespace content {
class ContentBrowserClient;
}

namespace meson {
class MainDelegate : public brightray::MainDelegate {
 public:
  MainDelegate();
  ~MainDelegate();

 protected:
  virtual bool BasicStartupComplete(int* exit_code) override;
  virtual void PreSandboxStartup(void) override;
  virtual std::unique_ptr<brightray::ContentClient> CreateContentClient() override;
  virtual std::unique_ptr<brightray::BrowserClient> CreateBrowserClient() override;

 protected:
  virtual content::ContentRendererClient* CreateContentRendererClient() override;
#if defined(OS_MACOSX)
  // Subclasses can override this to custom the paths of child process and
  // framework bundle.
  virtual void OverrideChildProcessPath();
  virtual void OverrideFrameworkBundlePath();
#endif

 private:
  DISALLOW_COPY_AND_ASSIGN(MainDelegate);
};
}
