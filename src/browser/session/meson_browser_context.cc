#include "browser/session/meson_browser_context.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/user_agent.h"

#include "app/common/chrome_version.h"
#include "app/common/meson_version.h"

#include "browser/browser.h"

static std::string RemoveWhitespace(const std::string& str) {
  std::string trimmed;
  if (base::RemoveChars(str, " ", &trimmed)) {
    return trimmed;
  } else {
    return str;
  }
}

namespace meson {
MesonBrowserContext::MesonBrowserContext(MesonSessionBinding* binding,
                                         const std::string& partition,
                                         bool is_memory,
                                         const base::DictionaryValue& args)
    : brightray::BrowserContext(partition, is_memory),
      binding_(binding->GetWeakPtr()),
      use_cache_(true) {
  Browser* browser = Browser::Get();
  std::string name = RemoveWhitespace(browser->GetName());
  std::string user_agent;
  if (name == MESON_PRODUCT_NAME) {
    user_agent = "Chrome/" CHROME_VERSION_STRING " " MESON_PRODUCT_NAME "/" MESON_VERSION_STRING;
  } else {
    user_agent = base::StringPrintf("%s/%s Chrome/%s " MESON_PRODUCT_NAME "/" MESON_VERSION_STRING,
                                    name.c_str(),
                                    browser->GetVersion().c_str(),
                                    CHROME_VERSION);
    args.GetBoolean("cache", &use_cache_);
  }
  //TODO: mada

  InitPrefs();
}

scoped_refptr<MesonBrowserContext> MesonBrowserContext::From(MesonSessionBinding* binding,
                                                             const std::string& partition,
                                                             bool is_memory,
                                                             const base::DictionaryValue& args) {
  auto browser_context = brightray::BrowserContext::Get(partition, is_memory);
  if (browser_context) {
    return static_cast<MesonBrowserContext*>(browser_context.get());
  }

  return new MesonBrowserContext(binding, partition, is_memory, args);
}

MesonBrowserContext::~MesonBrowserContext() {}
}
