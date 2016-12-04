//-*-c++-*-
#pragma once

#include <memory>

#include "brightray/common/main_delegate.h"
//#include "browser/browser_client.h"
//#include "renderer/renderer_client.h"

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
  virtual content::ContentBrowserClient* CreateContentBrowserClient() override;
  virtual content::ContentRendererClient* CreateContentRendererClient() override;
  virtual std::unique_ptr<brightray::ContentClient> CreateContentClient() override;
  int RunProcess(const std::string& process_type, const content::MainFunctionParams& main_function_params) override;

 protected:
#if defined(OS_MACOSX)
  // Subclasses can override this to custom the paths of child process and
  // framework bundle.
  bool ShouldSendMachPort(const std::string& process_type) override;
  bool DelaySandboxInitialization(const std::string& process_type) override;
  virtual void OverrideChildProcessPath();
  virtual void OverrideFrameworkBundlePath();
#endif

 private:
#if defined(OS_MACOSX)
  void SetUpBundleOverrides();
#endif
 private:
  std::unique_ptr<content::ContentBrowserClient> browser_client_;
  std::unique_ptr<content::ContentRendererClient> renderer_client_;
  //std::unique_ptr<content::ContentUtilityClient> utility_client_;

  DISALLOW_COPY_AND_ASSIGN(MainDelegate);
};
}
