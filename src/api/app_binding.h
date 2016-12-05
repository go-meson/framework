//-*-c++-*-
#pragma once
#include "api/api_binding.h"
#include "browser/browser_client.h"
#include "browser/browser_observer.h"

namespace meson {
class MesonAppBinding : public APIBindingT<MesonAppBinding>,
                        public MesonBrowserClient::Delegate,
                        public BrowserObserver {
 public:
  MesonAppBinding(unsigned int id);
  virtual ~MesonAppBinding(void);

 public:  // Local Methods
  MethodResult Exit(const api::APIArgs& args);
  MethodResult SetApplicationMenu(const api::APIArgs& args);
  MethodResult ShowMessageBox(const api::APIArgs& args);
  void ShowMessageBoxCallback(const std::string& eventName, int buttonId);
 public:  // BrowserObserver
  void OnBeforeQuit(bool* prevent_default) override;
  void OnWillQuit(bool* prevent_default) override;
  void OnWindowAllClosed() override;
  void OnQuit() override;
  void OnOpenFile(bool* prevent_default, const std::string& file_path) override;
  void OnOpenURL(const std::string& url) override;
  void OnActivate(bool has_visible_windows) override;
  void OnWillFinishLaunching() override;
  void OnFinishLaunching(const base::DictionaryValue& launch_info) override;
#if 0
  //TODO:
  void OnLogin(LoginHandler* login_handler, const base::DictionaryValue& request_details) override;
#endif
  void OnAccessibilitySupportChanged() override;
#if defined(OS_MACOSX)
  void OnContinueUserActivity(bool* prevent_default, const std::string& type, const base::DictionaryValue& user_info) override;
#endif
// content::ContentBrowserClient:
#if 0
  void AllowCertificateError(content::WebContents* web_contents,
                             int cert_error,
                             const net::SSLInfo& ssl_info,
                             const GURL& request_url,
                             content::ResourceType resource_type,
                             bool overridable,
                             bool strict_enforcement,
                             bool expired_previous_decision,
                             const base::Callback<void(bool)>& callback,
                             content::CertificateRequestResultType* request) override;
  void SelectClientCertificate(content::WebContents* web_contents,
                               net::SSLCertRequestInfo* cert_request_info,
                               std::unique_ptr<content::ClientCertificateDelegate> delegate) override;
#endif

 private:
  bool IsAccessibilitySupportEnabled();
};
}
