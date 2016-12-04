#include "browser_client.h"

#include "base/logging.h"
#include "base/command_line.h"
#include "common/options_switches.h"
#include "content/public/common/web_preferences.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "ui/base/l10n/l10n_util.h"
#include "net/ssl/ssl_cert_request_info.h"

#include "browser/browser_main_parts.h"
#include "browser/web_contents_preferences.h"

namespace meson {
namespace {
bool g_suppress_renderer_process_restart = false;
std::string g_custom_service_worker_schemes = "";
void Noop(scoped_refptr<content::SiteInstance>) {}
}

MesonBrowserClient::MesonBrowserClient()
    : delegate_(nullptr) {
  DLOG(INFO) << __PRETTY_FUNCTION__;
}
MesonBrowserClient::~MesonBrowserClient() {}

void MesonBrowserClient::RenderProcessWillLaunch(content::RenderProcessHost* host) {
  int process_id = host->GetID();
#if 0
  //TODO:
  host->AddFilter(new printing::PrintingMessageFilter(process_id));
  host->AddFilter(new TtsMessageFilter(process_id, host->GetBrowserContext()));
  host->AddFilter(new WidevineCdmMessageFilter(process_id, host->GetBrowserContext()));
#endif

  content::WebContents* web_contents = GetWebContentsFromProcessID(process_id);
  if (WebContentsPreferences::IsSandboxed(web_contents)) {
    AddSandboxedRendererId(host->GetID());
    // ensure the sandboxed renderer id is removed later
    host->AddObserver(this);
  }
}
void MesonBrowserClient::OverrideWebkitPrefs(content::RenderViewHost* host, content::WebPreferences* prefs) {
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
  prefs->allow_universal_access_from_file_urls = true;
  prefs->allow_file_access_from_file_urls = true;
  prefs->experimental_webgl_enabled = true;
  prefs->allow_displaying_insecure_content = false;
  prefs->allow_running_insecure_content = false;

  // Custom preferences of guest page.
  auto web_contents = content::WebContents::FromRenderViewHost(host);
  WebContentsPreferences::OverrideWebkitPrefs(web_contents, prefs);
}

#if 0
content::SpeechRecognitionManagerDelegate* MesonBrowserClient::CreateSpeechRecognitionManagerDelegate() {
  return new MesonSpeechRecognitionManagerDelegate;
}

content::GeolocationDelegate* MesonBrowserClient::CreateGeolocationDelegate() {
  return new MesonGeolocationDelegate();
}
#endif

std::string MesonBrowserClient::GetApplicationLocale() {
  return l10n_util::GetApplicationLocale("");
}

void MesonBrowserClient::OverrideSiteInstanceForNavigation(content::BrowserContext* browser_context,
                                                           content::SiteInstance* current_instance,
                                                           const GURL& url,
                                                           content::SiteInstance** new_instance) {
  if (g_suppress_renderer_process_restart) {
    g_suppress_renderer_process_restart = false;
    return;
  }

  if (!ShouldCreateNewSiteInstance(browser_context, current_instance, url))
    return;

  scoped_refptr<content::SiteInstance> site_instance =
      content::SiteInstance::CreateForURL(browser_context, url);
  *new_instance = site_instance.get();

  // Make sure the |site_instance| is not freed when this function returns.
  // FIXME(zcbenz): We should adjust OverrideSiteInstanceForNavigation's
  // interface to solve this.
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE, base::Bind(&Noop, base::RetainedRef(site_instance)));

  // Remember the original renderer process of the pending renderer process.
  auto current_process = current_instance->GetProcess();
  auto pending_process = (*new_instance)->GetProcess();
  pending_processes_[pending_process->GetID()] = current_process->GetID();
  // Clear the entry in map when process ends.
  current_process->AddObserver(this);
}

void MesonBrowserClient::AppendExtraCommandLineSwitches(base::CommandLine* command_line, int child_process_id) {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  std::string process_type = command_line->GetSwitchValueASCII("type");
  if (process_type != "renderer")
    return;

  // Copy following switches to child process.
  static const char* const kCommonSwitchNames[] = {switches::kStandardSchemes, switches::kEnableSandbox};
  command_line->CopySwitchesFrom(*base::CommandLine::ForCurrentProcess(), kCommonSwitchNames, arraysize(kCommonSwitchNames));

  // The registered service worker schemes.
  if (!g_custom_service_worker_schemes.empty())
    command_line->AppendSwitchASCII(switches::kRegisterServiceWorkerSchemes, g_custom_service_worker_schemes);

#if defined(OS_WIN)
  // Append --app-user-model-id.
  PWSTR current_app_id;
  if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&current_app_id))) {
    command_line->AppendSwitchNative(switches::kAppUserModelId, current_app_id);
    CoTaskMemFree(current_app_id);
  }
#endif

  content::WebContents* web_contents = GetWebContentsFromProcessID(child_process_id);
  if (!web_contents)
    return;

  WebContentsPreferences::AppendExtraCommandLineSwitches(web_contents, command_line);
}

#if 0
void MesonBrowserClient::DidCreatePpapiPlugin(content::BrowserPpapiHost* host) {
  host->GetPpapiHost()->AddHostFactoryFilter(base::WrapUnique(new chrome::ChromeBrowserPepperHostFactory(host)));
}

content::QuotaPermissionContext* MesonBrowserClient::CreateQuotaPermissionContext() {
  return new MesonQuotaPermissionContext;
}
#endif

void MesonBrowserClient::AllowCertificateError(content::WebContents* web_contents,
                                               int cert_error,
                                               const net::SSLInfo& ssl_info,
                                               const GURL& request_url,
                                               content::ResourceType resource_type,
                                               bool overridable,
                                               bool strict_enforcement,
                                               bool expired_previous_decision,
                                               const base::Callback<void(bool)>& callback,
                                               content::CertificateRequestResultType* request) {
  if (delegate_) {
    delegate_->AllowCertificateError(
        web_contents, cert_error, ssl_info, request_url,
        resource_type, overridable, strict_enforcement,
        expired_previous_decision, callback, request);
  }
}

void MesonBrowserClient::SelectClientCertificate(content::WebContents* web_contents,
                                                 net::SSLCertRequestInfo* cert_request_info,
                                                 std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  if (!cert_request_info->client_certs.empty() && delegate_) {
    delegate_->SelectClientCertificate(web_contents, cert_request_info, std::move(delegate));
  }
}

#if 0
  void MesonBrowserClient::ResourceDispatcherHostCreated() {
    resource_dispatcher_host_delegate_.reset(new MesonResourceDispatcherHostDelegate);
    content::ResourceDispatcherHost::Get()->SetDelegate(resource_dispatcher_host_delegate_.get());
  }
#endif

void MesonBrowserClient::GetAdditionalAllowedSchemesForFileSystem(std::vector<std::string>* additional_schemes) {
#if 0
  //TODO:
  auto schemes_list = api::GetStandardSchemes();
  if (!schemes_list.empty())
    additional_schemes->insert(additional_schemes->end(),
                               schemes_list.begin(),
                               schemes_list.end());
#endif
  additional_schemes->push_back(content::kChromeDevToolsScheme);
}

brightray::BrowserMainParts* MesonBrowserClient::OverrideCreateBrowserMainParts(const content::MainFunctionParams&) {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  return new MesonMainParts();
}

void MesonBrowserClient::WebNotificationAllowed(int render_process_id, const base::Callback<void(bool, bool)>& callback) {
  content::WebContents* web_contents = WebContentsPreferences::GetWebContentsFromProcessID(render_process_id);
  if (!web_contents) {
    callback.Run(false, false);
    return;
  }
#if 0
  //TODO:
  auto permission_helper = WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper) {
    callback.Run(false, false);
    return;
  }
  permission_helper->RequestWebNotificationPermission(base::Bind(callback, web_contents->IsAudioMuted()));
#else
  callback.Run(false, false);
#endif
}

content::WebContents* MesonBrowserClient::GetWebContentsFromProcessID(
    int process_id) {
  // If the process is a pending process, we should use the old one.
  if (ContainsKey(pending_processes_, process_id))
    process_id = pending_processes_[process_id];

  // Certain render process will be created with no associated render view,
  // for example: ServiceWorker.
  return WebContentsPreferences::GetWebContentsFromProcessID(process_id);
}

void MesonBrowserClient::AddSandboxedRendererId(int process_id) {
  base::AutoLock auto_lock(sandboxed_renderers_lock_);
  sandboxed_renderers_.insert(process_id);
}

void MesonBrowserClient::RemoveSandboxedRendererId(int process_id) {
  base::AutoLock auto_lock(sandboxed_renderers_lock_);
  sandboxed_renderers_.erase(process_id);
}

bool MesonBrowserClient::IsRendererSandboxed(int process_id) {
  base::AutoLock auto_lock(sandboxed_renderers_lock_);
  return sandboxed_renderers_.count(process_id);
}

void MesonBrowserClient::SuppressRendererProcessRestartForOnce() {
  g_suppress_renderer_process_restart = true;
}

bool MesonBrowserClient::ShouldCreateNewSiteInstance(content::BrowserContext* browser_context,
                                                     content::SiteInstance* current_instance,
                                                     const GURL& url) {
  if (url.SchemeIs(url::kJavaScriptScheme))
    // "javacript:" scheme should always use same SiteInstance
    return false;

  if (!IsRendererSandboxed(current_instance->GetProcess()->GetID()))
    // non-sandboxed renderers should always create a new SiteInstance
    return true;

  // Create new a SiteInstance if navigating to a different site.
  auto src_url = current_instance->GetSiteURL();
  return !content::SiteInstance::IsSameWebSite(browser_context, src_url, url) &&
         // `IsSameWebSite` doesn't seem to work for some URIs such as `file:`,
         // handle these scenarios by comparing only the site as defined by
         // `GetSiteForURL`.
         content::SiteInstance::GetSiteForURL(browser_context, url) != src_url;
}
}
