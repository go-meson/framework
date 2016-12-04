#include "browser/web_contents_preferences.h"

#include "base/command_line.h"
#include "net/base/filename_util.h"
#include "cc/base/switches.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_switches.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/common/web_preferences.h"

#include "common/options_switches.h"
#include "browser/native_window.h"
#include "browser/web_view_manager.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(meson::WebContentsPreferences);

namespace meson {
std::vector<WebContentsPreferences*> WebContentsPreferences::instances_;

WebContentsPreferences::WebContentsPreferences(content::WebContents* web_contents, const base::DictionaryValue& web_preferences)
    : web_contents_(web_contents), web_preferences_(web_preferences.DeepCopy()) {
  web_preferences_->Remove("embedder", nullptr);
  web_preferences_->Remove("isGuest", nullptr);
  web_preferences_->Remove("session", nullptr);

  web_contents->SetUserData(UserDataKey(), this);
  instances_.push_back(this);
}
WebContentsPreferences::~WebContentsPreferences() {
  instances_.erase(std::remove(instances_.begin(), instances_.end(), this),
                   instances_.end());
}
void WebContentsPreferences::Merge(const base::DictionaryValue& new_web_preferences) {
  web_preferences_->MergeDictionary(&new_web_preferences);
}

content::WebContents* WebContentsPreferences::GetWebContentsFromProcessID(int process_id) {
  for (auto preferences : instances_) {
    auto web_contents = preferences->web_contents_;
    if (web_contents->GetRenderProcessHost()->GetID() == process_id) {
      return web_contents;
    }
  }
  return nullptr;
}

void WebContentsPreferences::OverrideWebkitPrefs(content::WebContents* web_contents, content::WebPreferences* prefs) {
  WebContentsPreferences* self = FromWebContents(web_contents);
  if (!self)
    return;

  bool b;
  if (self->web_preferences_->GetBoolean("javascript", &b))
    prefs->javascript_enabled = b;
  if (self->web_preferences_->GetBoolean("images", &b))
    prefs->images_enabled = b;
  if (self->web_preferences_->GetBoolean("textAreasAreResizable", &b))
    prefs->text_areas_are_resizable = b;
  if (self->web_preferences_->GetBoolean("webgl", &b))
    prefs->experimental_webgl_enabled = b;
  if (self->web_preferences_->GetBoolean("webSecurity", &b)) {
    prefs->web_security_enabled = b;
    prefs->allow_displaying_insecure_content = !b;
    prefs->allow_running_insecure_content = !b;
  }
  if (self->web_preferences_->GetBoolean("allowDisplayingInsecureContent", &b))
    prefs->allow_displaying_insecure_content = b;
  if (self->web_preferences_->GetBoolean("allowRunningInsecureContent", &b))
    prefs->allow_running_insecure_content = b;
  const base::DictionaryValue* fonts = nullptr;
  if (self->web_preferences_->GetDictionary("defaultFontFamily", &fonts)) {
    base::string16 font;
    if (fonts->GetString("standard", &font))
      prefs->standard_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("serif", &font))
      prefs->serif_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("sansSerif", &font))
      prefs->sans_serif_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("monospace", &font))
      prefs->fixed_font_family_map[content::kCommonScript] = font;
  }
  int size;
  if (self->web_preferences_->GetInteger("defaultFontSize", &size))
    prefs->default_font_size = size;
  if (self->web_preferences_->GetInteger("defaultMonospaceFontSize", &size))
    prefs->default_fixed_font_size = size;
  if (self->web_preferences_->GetInteger("minimumFontSize", &size))
    prefs->minimum_font_size = size;
  std::string encoding;
  if (self->web_preferences_->GetString("defaultEncoding", &encoding))
    prefs->default_encoding = encoding;
}

void WebContentsPreferences::AppendExtraCommandLineSwitches(content::WebContents* web_contents, base::CommandLine* command_line) {
  auto self = FromWebContents(web_contents);
  if (!self)
    return;
  auto& web_preferences = *self->web_preferences_;

  bool b;
  if (web_preferences.GetBoolean("plugins", &b) && b)
    command_line->AppendSwitch(switches::kEnablePlugins);

  if (web_preferences.GetBoolean(options::kExperimentalFeatures, &b) && b)
    command_line->AppendSwitch(::switches::kEnableExperimentalWebPlatformFeatures);
  if (web_preferences.GetBoolean(options::kExperimentalCanvasFeatures, &b) && b)
    command_line->AppendSwitch(::switches::kEnableExperimentalCanvasFeatures);

  if (IsSandboxed(web_contents))
    command_line->AppendSwitch(switches::kEnableSandbox);

  base::FilePath::StringType preload;
  if (web_preferences.GetString(switches::kPreloadScript, &preload)) {
    if (base::FilePath(preload).IsAbsolute())
      command_line->AppendSwitchNative(switches::kPreloadScript, preload);
    else
      LOG(ERROR) << "preload script must have absolute path.";
  } else if (web_preferences.GetString(options::kPreloadURL, &preload)) {
    base::FilePath preload_path;
    if (net::FileURLToFilePath(GURL(preload), &preload_path))
      command_line->AppendSwitchPath(switches::kPreloadScript, preload_path);
    else
      LOG(ERROR) << "preload url must be file:// protocol.";
  }

  // --background-color.
  std::string color;
  if (web_preferences.GetString(options::kBackgroundColor, &color))
    command_line->AppendSwitchASCII(switches::kBackgroundColor, color);

  // The zoom factor.
  double zoom_factor = 1.0;
  if (web_preferences.GetDouble(options::kZoomFactor, &zoom_factor) &&
      zoom_factor != 1.0)
    command_line->AppendSwitchASCII(switches::kZoomFactor,
                                    base::DoubleToString(zoom_factor));

  // --guest-instance-id, which is used to identify guest WebContents.
  int guest_instance_id = 0;
  if (web_preferences.GetInteger(options::kGuestInstanceID, &guest_instance_id))
    command_line->AppendSwitchASCII(switches::kGuestInstanceID,
                                    base::IntToString(guest_instance_id));

  // Pass the opener's window id.
  int opener_id;
  if (web_preferences.GetInteger(options::kOpenerID, &opener_id))
    command_line->AppendSwitchASCII(switches::kOpenerID,
                                    base::IntToString(opener_id));

#if defined(OS_MACOSX)
  // Enable scroll bounce.
  bool scroll_bounce;
  if (web_preferences.GetBoolean(options::kScrollBounce, &scroll_bounce) &&
      scroll_bounce)
    command_line->AppendSwitch(switches::kScrollBounce);
#endif

  // Custom command line switches.
  const base::ListValue* args;
  if (web_preferences.GetList("commandLineSwitches", &args)) {
    for (size_t i = 0; i < args->GetSize(); ++i) {
      std::string arg;
      if (args->GetString(i, &arg) && !arg.empty())
        command_line->AppendSwitch(arg);
    }
  }

  // Enable blink features.
  std::string blink_features;
  if (web_preferences.GetString(options::kBlinkFeatures, &blink_features))
    command_line->AppendSwitchASCII(::switches::kEnableBlinkFeatures,
                                    blink_features);

  // Disable blink features.
  std::string disable_blink_features;
  if (web_preferences.GetString(options::kDisableBlinkFeatures,
                                &disable_blink_features))
    command_line->AppendSwitchASCII(::switches::kDisableBlinkFeatures,
                                    disable_blink_features);

  // The initial visibility state.
  NativeWindow* window = NativeWindow::FromWebContents(web_contents);

  // Use embedder window for webviews
  if (guest_instance_id && !window) {
    auto manager = WebViewManager::GetWebViewManager(web_contents);
    if (manager) {
      auto embedder = manager->GetEmbedder(guest_instance_id);
      if (embedder)
        window = NativeWindow::FromWebContents(embedder);
    }
  }

  if (window) {
    bool visible = window->IsVisible() && !window->IsMinimized();
    if (!visible)  // Default state is visible.
      command_line->AppendSwitch("hidden-page");
  }

  // Use frame scheduling for offscreen renderers.
  // TODO(zcbenz): Remove this after Chrome 54, on which it becomes default.
  bool offscreen;
  if (web_preferences.GetBoolean("offscreen", &offscreen) && offscreen)
    command_line->AppendSwitch(cc::switches::kEnableBeginFrameScheduling);
}

bool WebContentsPreferences::IsSandboxed(content::WebContents* web_contents) {
  WebContentsPreferences* self;
  if (!web_contents)
    return false;

  self = FromWebContents(web_contents);
  if (!self)
    return false;

  auto& web_preferences = *self->web_preferences_;
  bool b = false;
  web_preferences.GetBoolean("sandbox", &b);
  return b;
}
}
