//-*-c++-*-
#pragma once

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "brightray/browser/browser_context.h"
#include "api/session_binding.h"

#include "browser/web_view_manager.h"

namespace meson {
class MesonSessionBinding;
class MesonBrowserContext : public brightray::BrowserContext {
 public:
  MesonBrowserContext(MesonSessionBinding* binding, const std::string& partition, bool is_memory, const base::DictionaryValue& args);
  virtual ~MesonBrowserContext(void);

 public:
  static scoped_refptr<MesonBrowserContext> From(MesonSessionBinding* binding,
                                                 const std::string& partition,
                                                 bool is_memory,
                                                 const base::DictionaryValue& args);

 public:
  content::BrowserPluginGuestManager* GetGuestManager() override;

 private:
  base::WeakPtr<APIBinding> binding_;
  std::string user_agent_;
  bool use_cache_;
  std::unique_ptr<WebViewManager> guest_manager_;

  DISALLOW_COPY_AND_ASSIGN(MesonBrowserContext);
};
}
