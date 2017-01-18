#include "renderer_client.h"

#include "base/logging.h"
#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/sys_string_conversions.h"
#include "url/url_util.h"
#if defined(OS_MACOSX)
#include "base/mac/scoped_cftyperef.h"
#endif

#include "content/public/common/content_constants.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"

#include "third_party/WebKit/public/web/WebCustomElement.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrameWidget.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebPluginParams.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"

#include "renderer/extensions/safe_builtins.h"
#include "renderer/extensions/script_context.h"
#include "renderer/extensions/module_system.h"

#include "common/color_util.h"
#include "common/options_switches.h"
#include "renderer/content_settings_observer.h"
#include "renderer/meson_render_view_observer.h"
#include "renderer/meson_render_frame_observer.h"
#include "renderer/preferences_manager.h"
#include "renderer/guest_view_container.h"
#include "renderer/extensions/document_bindings.h"
#include "renderer/extensions/web_view_bindings.h"
#include "renderer/extensions/remote_bindings.h"

namespace meson {
namespace {
bool IsDevToolsExtension(content::RenderFrame* render_frame) {
  return static_cast<GURL>(render_frame->GetWebFrame()->document().url()).SchemeIs("chrome-extension");
}
}

MesonRendererClient::MesonRendererClient() {
  LOG(INFO) << __PRETTY_FUNCTION__;
  auto command_line = base::CommandLine::ForCurrentProcess();
  auto custom_schemes = command_line->GetSwitchValueASCII(switches::kStandardSchemes);
  if (!custom_schemes.empty()) {
    auto schemes_list = base::SplitString(custom_schemes, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    for (auto& scheme : schemes_list) {
      url::AddStandardScheme(scheme.c_str(), url::SCHEME_WITHOUT_PORT);
    }
  }
}

MesonRendererClient::~MesonRendererClient() {
}

void MesonRendererClient::RenderThreadStarted() {
  LOG(INFO) << __PRETTY_FUNCTION__;
  blink::WebCustomElement::addEmbedderCustomElementName("webview");
  blink::WebCustomElement::addEmbedderCustomElementName("browserplugin");

  preferences_manager_.reset(new PreferencesManager);

//TODO:
#if defined(OS_WIN)
  // Set ApplicationUserModelID in renderer process.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  base::string16 app_id = command_line->GetSwitchValueNative(switches::kAppUserModelId);
  if (!app_id.empty()) {
    SetCurrentProcessExplicitAppUserModelID(app_id.c_str());
  }
#endif

#if defined(OS_MACOSX)
  // Disable rubber banding by default.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switches::kScrollBounce)) {
    base::ScopedCFTypeRef<CFStringRef> key(base::SysUTF8ToCFStringRef("NSScrollViewRubberbanding"));
    base::ScopedCFTypeRef<CFStringRef> value(base::SysUTF8ToCFStringRef("false"));
    CFPreferencesSetAppValue(key, value, kCFPreferencesCurrentApplication);
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
  }
#endif

  auto thread = content::RenderThread::Get();
  thread->RegisterExtension(extensions::SafeBuiltins::CreateV8Extension());

  static const
#include "renderer/resources/extensions/web_view.js.bin"
      std::string
      web_view_src(reinterpret_cast<const char*>(src_renderer_resources_extensions_web_view_js), src_renderer_resources_extensions_web_view_js_len);
  source_map_.RegisterSource("webview", web_view_src);

  static const
#include "renderer/resources/extensions/remote.js.bin"
      std::string
      remote_src(reinterpret_cast<const char*>(src_renderer_resources_extensions_remote_js), src_renderer_resources_extensions_remote_js_len);
  source_map_.RegisterSource("remote", remote_src);
}

void MesonRendererClient::RenderFrameCreated(content::RenderFrame* render_frame) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  new MesonRenderFrameObserver(render_frame, this);
  new ContentSettingsObserver(render_frame);

  blink::WebSecurityPolicy::registerURLSchemeAsAllowingServiceWorkers("file");
}

void MesonRendererClient::RenderViewCreated(content::RenderView* render_view) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  new MesonRenderViewObserver(render_view, this);

  auto* web_frame_widget = render_view->GetWebFrameWidget();
  if (!web_frame_widget)
    return;

  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch(switches::kGuestInstanceID)) {
    // WebView
    web_frame_widget->setBaseBackgroundColor(SK_ColorTRANSPARENT);
  } else {
    // Normal Window
    auto name = cmd->GetSwitchValueASCII(switches::kBackgroundColor);
    auto color = name.empty() ? SK_ColorWHITE : ParseHexColor(name);
    web_frame_widget->setBaseBackgroundColor(color);
  }
}

void MesonRendererClient::DidClearWindowObject(content::RenderFrame* render_frame) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  render_frame->GetWebFrame()->executeScript(blink::WebScriptSource("void 0"));
}

#if 0
void MesonRendererClient::RunScriptsAtDocumentStart(content::RenderFrame* render_frame) {
  LOG(INFO) << __PRETTY_FUNCTION__;
}

void MesonRendererClient::RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) {
  LOG(INFO) << __PRETTY_FUNCTION__;
}
#endif
#if 0
  blink::WebSpeechSynthesizer* AtomRendererClient::OverrideSpeechSynthesizer(
                                                                             blink::WebSpeechSynthesizerClient* client) {
    return new TtsDispatcher(client);
  }
#endif

bool MesonRendererClient::OverrideCreatePlugin(content::RenderFrame* render_frame,
                                               blink::WebLocalFrame* frame,
                                               const blink::WebPluginParams& params,
                                               blink::WebPlugin** plugin) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << params.mimeType.utf8();
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (params.mimeType.utf8() == content::kBrowserPluginMimeType ||
      command_line->HasSwitch(switches::kEnablePlugins))
    return false;

  *plugin = nullptr;
  return true;
}

bool MesonRendererClient::ShouldFork(blink::WebLocalFrame* frame,
                                     const GURL& url,
                                     const std::string& http_method,
                                     bool is_initial_navigation,
                                     bool is_server_redirect,
                                     bool* send_referrer) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  *send_referrer = true;
  return http_method == "GET";
}

content::BrowserPluginDelegate* MesonRendererClient::CreateBrowserPluginDelegate(content::RenderFrame* render_frame,
                                                                                 const std::string& mime_type,
                                                                                 const GURL& original_url) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << mime_type << ", " << original_url.spec() << ")";
  if (mime_type == content::kBrowserPluginMimeType) {
    return new GuestViewContainer(render_frame);
  } else {
    return nullptr;
  }
}

void MesonRendererClient::DidCreateScriptContext(v8::Handle<v8::Context> context, content::RenderFrame* render_frame) {
  using extensions::ScriptContext;
  using extensions::ModuleSystem;
  if (!render_frame->IsMainFrame() && !IsDevToolsExtension(render_frame))
    return;

  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << render_frame->GetWebFrame()->uniqueName().utf8() << ") " << render_frame->IsMainFrame() << ":" << IsDevToolsExtension(render_frame);
  ScriptContext* script_context = new ScriptContext(context, render_frame->GetWebFrame());
  {
    std::unique_ptr<ModuleSystem> module_system(new ModuleSystem(script_context, &source_map_));
    script_context->set_module_system(std::move(module_system));
  }
  auto* module_system = script_context->module_system();

  // Enable natives in startup.
  ModuleSystem::NativesEnabledScope natives_enabled_scope(module_system);

  module_system->RegisterNativeHandler(
      "document_natives",
      std::unique_ptr<extensions::DocumentBindings>(new extensions::DocumentBindings(script_context)));
  module_system->RegisterNativeHandler(
      "webview_natives",
      std::unique_ptr<extensions::WebViewBindings>(new extensions::WebViewBindings(script_context)));
  module_system->RegisterNativeHandler(
      "remote_natives",
      std::unique_ptr<extensions::RemoteBindings>(new extensions::RemoteBindings(script_context)));

  module_system->Require("webview");
  module_system->Require("remote");
}

void MesonRendererClient::WillReleaseScriptContext(v8::Handle<v8::Context> context, content::RenderFrame* render_frame) {
  // Only allow node integration for the main frame, unless it is a devtools
  // extension page.
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << render_frame->IsMainFrame() << " / "<< IsDevToolsExtension(render_frame);
  if (!render_frame->IsMainFrame() && !IsDevToolsExtension(render_frame))
    return;

#if 0
  //TODO: atode
  node::Environment* env = node::Environment::GetCurrent(context);
  if (env)
    mate::EmitEvent(env->isolate(), env->process_object(), "exit");
#endif
}
}
