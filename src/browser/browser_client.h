#pragma once

#include "base/compiler_specific.h"
#include "brightray/browser/browser_client.h"

namespace meson {
class MesonBrowserClient : public brightray::BrowserClient {
 public:
  MesonBrowserClient();
  virtual ~MesonBrowserClient();

 public:
  virtual void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                              int child_process_id) override;
  virtual void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                                   content::WebPreferences* prefs) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MesonBrowserClient);
};
}