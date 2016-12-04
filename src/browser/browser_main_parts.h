//-*-c++-*-
#pragma once

#include <memory>
#include "brightray/browser/browser_main_parts.h"

#include "base/threading/thread.h"
#include "content/public/browser/browser_main_parts.h"
#include "base/memory/ref_counted.h"
#include "browser/browser.h"

#include "api/api.h"

namespace meson {
class APIServer;
class Browser;
class MesonMainParts : public brightray::BrowserMainParts {
 public:
  explicit MesonMainParts();
  virtual ~MesonMainParts();
  static MesonMainParts* Get(void);

 public:
  void PreMainMessageLoopRun(void) override;
  void PostMainMessageLoopRun(void) override;
  bool MainMessageLoopRun(int* result_code) override;
#if defined(OS_MACOSX)
  void PreMainMessageLoopStart() override;
  void FreeAppDelegate();
#endif

  Browser* browser() { return browser_.get(); }
  bool SetExitCode(int exit_code);
  int GetExitCode() const;

 private:
  static MesonMainParts* self_;
  std::unique_ptr<Browser> browser_;
  std::unique_ptr<API> api_;
  scoped_refptr<APIServer> api_server_;
  int* exit_code_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MesonMainParts);
};
}
