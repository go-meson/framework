#include "api/app_binding.h"
#include "browser/browser_main_parts.h"
#include "browser/browser_client.h"
#include "api/web_contents_binding.h"
#include "api/menu_binding.h"
#include "api/api.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "browser/ui/message_box.h"

namespace meson {
template <>
APIBindingT<MesonAppBinding>::MethodTable APIBindingT<MesonAppBinding>::methodTable = {
    {"exit", std::mem_fn(&MesonAppBinding::Exit)},
#if defined(OS_MACOSX)
    {"setApplicationMenu", std::mem_fn(&MesonAppBinding::SetApplicationMenu)},
#endif
    {"showMessageBox", std::mem_fn(&MesonAppBinding::ShowMessageBox)},
};

MesonAppBinding::MesonAppBinding(unsigned int id)
    : APIBindingT<meson::MesonAppBinding>(MESON_OBJECT_TYPE_APP, id) {
  static_cast<MesonBrowserClient*>(MesonBrowserClient::Get())->set_delegate(this);
  Browser::Get()->AddObserver(this);
#if 0
  //TODO:
  content::GpuDataManager::GetInstance()->AddObserver(this);
#endif
}
MesonAppBinding::~MesonAppBinding() {
  LOG(INFO) << __PRETTY_FUNCTION__;
}

MesonAppBinding::MethodResult MesonAppBinding::Exit(const api::APIArgs& args) {
  int exitCode = 0;
  args.GetInteger(0, &exitCode);
  Browser::Get()->Exit(exitCode);

  return MethodResult();
}

#if defined(OS_MACOSX)
MesonAppBinding::MethodResult MesonAppBinding::SetApplicationMenu(const api::APIArgs& args) {
  int id;
  args.GetInteger(0, &id);
  auto menu = API::Get()->GetBinding<MesonMenuBinding>(id);
  if (!menu) {
    return MethodResult("invalid menu id");
  }
  MesonMenuBinding::SetApplicationMenu(menu.get());
  return MethodResult();
}
#endif

void MesonAppBinding::ShowMessageBoxCallback(const std::string& eventName, int buttonId) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << " : " << eventName << ", " << buttonId;
  EmitEvent(eventName, buttonId);
}
MesonAppBinding::MethodResult MesonAppBinding::ShowMessageBox(const api::APIArgs& args) {
  int window_id = -1;
  const base::DictionaryValue* opt = nullptr;
  std::string callback_event_name;
  if (!args.GetInteger(0, &window_id) || !args.GetDictionary(1, &opt)) {
    return MethodResult("invalid argument");
  }
  args.GetString(2, &callback_event_name);
  NativeWindow* parent_window = nullptr;

  if (window_id != 0) {
    auto win = API::Get()->GetBinding<MesonWindowBinding>(window_id);
    if (!win) {
      return MethodResult("invalid parent window");
    }
    parent_window = win->window();
  }

  int type = MESSAGE_BOX_TYPE_NONE;
  int default_id = -1;
  int cancel_id = -1;
  int options = 0;
  const base::ListValue* p;
  std::string title, message, detail;
  //const gfx::ImageSkia& icon,
  //atom::NativeWindow* window,
  opt->GetInteger("type", &type);
  std::vector<std::string> buttons;
  if (opt->GetList("buttons", &p)) {
    buttons.resize(p->GetSize());
    for (size_t idx = 0; idx < p->GetSize(); idx++) {
      p->GetString(idx, &buttons[idx]);
    }
  }
  opt->GetString("title", &title);
  opt->GetString("message", &message);
  opt->GetString("detail", &detail);
  opt->GetInteger("defaultId", &default_id);
  opt->GetInteger("cancelId", &cancel_id);
  opt->GetInteger("option", &options);

  gfx::ImageSkia icon;
  if (callback_event_name.empty()) {
    int ret = ::meson::ShowMessageBox(parent_window, static_cast<MessageBoxType>(type), buttons, default_id, cancel_id, options, title, message, detail, icon);
    return MethodResult(std::unique_ptr<base::Value>(new base::FundamentalValue(ret)));
  } else {
    MessageBoxCallback callback = base::Bind(&MesonAppBinding::ShowMessageBoxCallback, this, callback_event_name);
    ::meson::ShowMessageBox(parent_window, static_cast<MessageBoxType>(type), buttons, default_id, cancel_id, options, title, message, detail, icon, callback);
    return MethodResult();
  }
}

void MesonAppBinding::OnBeforeQuit(bool* prevent_default) {
  bool prevent = EmitPreventEvent("before-quit");
  *prevent_default = prevent;
}

void MesonAppBinding::OnWillQuit(bool* prevent_default) {
  bool prevent = EmitPreventEvent("will-quit");
  *prevent_default = prevent;
}

void MesonAppBinding::OnWindowAllClosed() {
  EmitEvent("window-all-closed");
}

void MesonAppBinding::OnQuit() {
  int exitCode = MesonMainParts::Get()->GetExitCode();
  EmitEvent("quit", exitCode);

  API::Get()->RemoveBinding(this);

#if 0
  if (process_singleton_.get()) {
    process_singleton_->Cleanup();
    process_singleton_.reset();
  }
#endif
}

void MesonAppBinding::OnOpenFile(bool* prevent_default, const std::string& file_path) {
  bool prevent = EmitPreventEvent("open-file", file_path);
  *prevent_default = prevent;
}

void MesonAppBinding::OnOpenURL(const std::string& url) {
  EmitEvent("open-url", url);
}

void MesonAppBinding::OnActivate(bool has_visible_windows) {
  EmitEvent("activate", has_visible_windows);
}

void MesonAppBinding::OnWillFinishLaunching() {
  EmitEvent("will-finish-launching");
}

void MesonAppBinding::OnFinishLaunching(const base::DictionaryValue& launch_info) {
#if defined(OS_LINUX)
  // Set the application name for audio streams shown in external
  // applications. Only affects pulseaudio currently.
  media::AudioManager::SetGlobalAppName(Browser::Get()->GetName());
#endif
  EmitEvent("ready", &launch_info);
}

//TODO:
#if 0
void MesonAppBinding::OnLogin(LoginHandler* login_handler, const base::DictionaryValue& request_details) {
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

void MesonAppBinding::OnAccessibilitySupportChanged() {
  EmitEvent("accessibility-support-changed", IsAccessibilitySupportEnabled());
}

#if defined(OS_MACOSX)
void MesonAppBinding::OnContinueUserActivity(bool* prevent_default, const std::string& type, const base::DictionaryValue& user_info) {
  *prevent_default = EmitPreventEvent("continue-activity", type, &user_info);
}
#endif

#if 0
void MesonAppBinding::AllowCertificateError(content::WebContents* web_contents,
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
            request_url);
            //net::ErrorToString(cert_error),
            //ssl_info.cert,
            //callback);

  // Deny the certificate by default.
  if (!prevent_default)
    *request = content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY;
}

void MesonAppBinding::SelectClientCertificate(content::WebContents* web_contents,
                                              net::SSLCertRequestInfo* cert_request_info,
                                              std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  std::shared_ptr<content::ClientCertificateDelegate> shared_delegate(delegate.release());
  //TODO:
  bool prevent_default = false;

  Emit("select-client-certificate",);
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

bool MesonAppBinding::IsAccessibilitySupportEnabled() {
  auto ax_state = content::BrowserAccessibilityState::GetInstance();
  return ax_state->IsAccessibleBrowser();
}
}
