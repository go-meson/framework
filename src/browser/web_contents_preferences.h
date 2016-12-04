//-*-c++-*-
#pragma once

#include <memory>
#include <vector>
#include "base/values.h"
#include "content/public/browser/web_contents_user_data.h"

namespace base {
class CommandLine;
}
namespace content {
struct WebPreferences;
}
namespace meson {
class WebContentsPreferences : public content::WebContentsUserData<WebContentsPreferences> {
 public:
  WebContentsPreferences(content::WebContents* web_contents, const base::DictionaryValue& web_preferences);
  ~WebContentsPreferences() override;

 public:
  static content::WebContents* GetWebContentsFromProcessID(int process_id);
  static void OverrideWebkitPrefs(content::WebContents* web_contents, content::WebPreferences* prefs);

 public:
  void Merge(const base::DictionaryValue& new_web_preferences);
  static void AppendExtraCommandLineSwitches(content::WebContents* web_contents,
                                             base::CommandLine* command_line);
  static bool IsSandboxed(content::WebContents* web_contents);
  base::DictionaryValue* web_preferences() { return web_preferences_.get(); }

 private:
  static std::vector<WebContentsPreferences*> instances_;
  content::WebContents* web_contents_;
  std::unique_ptr<base::DictionaryValue> web_preferences_;
};
}
