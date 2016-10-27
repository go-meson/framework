#include "browser_client.h"

#include "base/command_line.h"
#include "content/public/common/web_preferences.h"
#include "content/public/common/content_switches.h"

namespace meson {
MesonBrowserClient::MesonBrowserClient() {}
MesonBrowserClient::~MesonBrowserClient() {}

void MesonBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  /* Default behaviour. */
  command_line->AppendSwitch(switches::kEnableThreadedCompositing);
  //  command_line->AppendSwitch(switches::kForceOverlayFullscreenVideo);
}

void MesonBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* render_view_host,
    content::WebPreferences* prefs) {
  prefs->javascript_enabled = true;
  prefs->web_security_enabled = true;
  prefs->javascript_can_open_windows_automatically = true;
  prefs->plugins_enabled = true;
  prefs->dom_paste_enabled = true;
  prefs->allow_scripts_to_close_windows = true;
  prefs->javascript_can_access_clipboard = true;
  prefs->local_storage_enabled = true;
  prefs->databases_enabled = true;
  prefs->application_cache_enabled = true;
  prefs->allow_file_access_from_file_urls = true;
  prefs->allow_universal_access_from_file_urls = true;
  prefs->experimental_webgl_enabled = true;
  //    prefs->web_security_enabled = false;
}
}