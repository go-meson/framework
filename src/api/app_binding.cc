#include "api/app_binding.h"
#include "browser/browser_main_parts.h"
#include "browser/browser_client.h"
#include "api/web_contents_binding.h"
#include "api/api.h"
#include "content/public/browser/browser_accessibility_state.h"

namespace meson {
template <>
const APIBindingT<AppClassBinding, AppClassBinding>::MethodTable APIBindingT<AppClassBinding, AppClassBinding>::methodTable = {};

template <>
const APIClassBindingT<AppClassBinding, AppClassBinding>::MethodTable APIClassBindingT<AppClassBinding, AppClassBinding>::staticMethodTable = {
    {"exit", std::mem_fn(&AppClassBinding::Exit)},
};

MESON_IMPLEMENT_API_CLASS(AppClassBinding, AppClassBinding);

AppClassBinding::AppClassBinding(void)
    : APIClassBindingT<AppClassBinding, AppClassBinding>(MESON_OBJECT_TYPE_APP) {
  static_cast<MesonBrowserClient*>(MesonBrowserClient::Get())->set_delegate(this);
  Browser::Get()->AddObserver(this);
#if 0
  //TODO:
  content::GpuDataManager::GetInstance()->AddObserver(this);
#endif
}
AppClassBinding::~AppClassBinding() {
  LOG(INFO) << __PRETTY_FUNCTION__;
}

api::MethodResult AppClassBinding::Exit(const api::APIArgs& args) {
  int exitCode = 0;
  args.GetInteger(0, &exitCode);
  Browser::Get()->Exit(exitCode);

  return api::MethodResult();
}


void AppClassBinding::OnBeforeQuit(bool* prevent_default) {
  bool prevent = EmitPreventEvent("before-quit");
  *prevent_default = prevent;
}

void AppClassBinding::OnWillQuit(bool* prevent_default) {
  bool prevent = EmitPreventEvent("will-quit");
  *prevent_default = prevent;
}

void AppClassBinding::OnWindowAllClosed() {
  EmitEvent("window-all-closed");
}

void AppClassBinding::OnQuit() {
  int exitCode = MesonMainParts::Get()->GetExitCode();
  EmitEvent("quit", "exitCode", exitCode);

#if 0
  //API::Get()->RemoveBinding(this);
#endif

#if 0
  if (process_singleton_.get()) {
    process_singleton_->Cleanup();
    process_singleton_.reset();
  }
#endif
}

void AppClassBinding::OnOpenFile(bool* prevent_default, const std::string& file_path) {
  bool prevent = EmitPreventEvent("open-file", "filePath", file_path);
  *prevent_default = prevent;
}

void AppClassBinding::OnOpenURL(const std::string& url) {
  EmitEvent("open-url", "url", url);
}

void AppClassBinding::OnActivate(bool has_visible_windows) {
  EmitEvent("activate", "hasVisibleWindow", has_visible_windows);
}

void AppClassBinding::OnWillFinishLaunching() {
  EmitEvent("will-finish-launching");
}

void AppClassBinding::OnFinishLaunching(const base::DictionaryValue& launch_info) {
#if defined(OS_LINUX)
  // Set the application name for audio streams shown in external
  // applications. Only affects pulseaudio currently.
  media::AudioManager::SetGlobalAppName(Browser::Get()->GetName());
#endif
  EmitEvent("ready", "launchInfo", &launch_info);
}

//TODO:
#if 0
void AppClassBinding::OnLogin(LoginHandler* login_handler, const base::DictionaryValue& request_details) {
#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
#endif
  LOG(INFO) << __PRETTY_FUNCTION__;
  //TODO:
  bool prevent_default = false;
  Emit("login",
      WebContents::CreateFrom(isolate(), login_handler->GetWebContents()),
      request_details,
      login_handler->auth_info(),
      base::Bind(&PassLoginInformation, make_scoped_refptr(login_handler)));

  // Default behavior is to always cancel the auth.
  if (!prevent_default)
    login_handler->CancelAuth();
}
#endif

void AppClassBinding::OnAccessibilitySupportChanged() {
  EmitEvent("accessibility-support-changed",
            "IsAccessibilitySupportEnabled", IsAccessibilitySupportEnabled());
}

#if defined(OS_MACOSX)
void AppClassBinding::OnContinueUserActivity(bool* prevent_default, const std::string& type, const base::DictionaryValue& user_info) {
  *prevent_default = EmitPreventEvent("continue-activity",
                                      "type", type,
                                      "userInfo", &user_info);
}
#endif

#if 0
void AppClassBinding::AllowCertificateError(content::WebContents* web_contents,
                                            int cert_error,
                                            const net::SSLInfo& ssl_info,
                                            const GURL& request_url,
                                            content::ResourceType resource_type,
                                            bool overridable,
                                            bool strict_enforcement,
                                            bool expired_previous_decision,
                                            const base::Callback<void(bool)>& callback,
                                            content::CertificateRequestResultType* request) {
#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
#endif
  //TODO:
  bool prevent_default = false;
  EmitEvent("certificate-error",
            //WebContents::CreateFrom(isolate(), web_contents),
            "requestURL", request_url);
            //net::ErrorToString(cert_error),
            //ssl_info.cert,
            //callback);

  // Deny the certificate by default.
  if (!prevent_default)
    *request = content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY;
}

void AppClassBinding::SelectClientCertificate(content::WebContents* web_contents,
                                              net::SSLCertRequestInfo* cert_request_info,
                                              std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  std::shared_ptr<content::ClientCertificateDelegate> shared_delegate(delegate.release());
  //TODO:
  bool prevent_default = false;

  Emit("select-client-certificate");
#if 0
           WebContents::CreateFrom(isolate(), web_contents),
           cert_request_info->host_and_port.ToString(),
           cert_request_info->client_certs,
           base::Bind(&OnClientCertificateSelected,
                      isolate(),
                      shared_delegate));
#endif

  // Default to first certificate from the platform store.
  if (!prevent_default)
    shared_delegate->ContinueWithCertificate(
        cert_request_info->client_certs[0].get());
}
#endif

bool AppClassBinding::IsAccessibilitySupportEnabled() {
  auto ax_state = content::BrowserAccessibilityState::GetInstance();
  return ax_state->IsAccessibleBrowser();
}
}
