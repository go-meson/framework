//-*-c++-*-
#pragma once

#include <map>
#include <set>
#include "base/compiler_specific.h"
#include "brightray/browser/browser_client.h"
#include "content/public/browser/render_process_host_observer.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"

namespace meson {
class MesonSession;
class MesonBrowserClient : public brightray::BrowserClient,
                           public content::RenderProcessHostObserver {
 public:
  using Delegate = content::ContentBrowserClient;
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

 public:
  MesonBrowserClient();
  virtual ~MesonBrowserClient();

 public:
  content::WebContents* GetWebContentsFromProcessID(int process_id);

 protected:
  // content::ContentBrowserClient:
  void RenderProcessWillLaunch(content::RenderProcessHost* host) override;
  //content::SpeechRecognitionManagerDelegate* CreateSpeechRecognitionManagerDelegate() override;
  //content::GeolocationDelegate* CreateGeolocationDelegate() override;
  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host, content::WebPreferences* prefs) override;
  std::string GetApplicationLocale() override;
  void OverrideSiteInstanceForNavigation(content::BrowserContext* browser_context,
                                         content::SiteInstance* current_instance,
                                         const GURL& dest_url,
                                         content::SiteInstance** new_instance) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line, int child_process_id) override;
  //void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  //content::QuotaPermissionContext* CreateQuotaPermissionContext() override;
  void AllowCertificateError(content::WebContents* web_contents,
                             int cert_error,
                             const net::SSLInfo& ssl_info,
                             const GURL& request_url,
                             content::ResourceType resource_type,
                             bool overridable,
                             bool strict_enforcement,
                             bool expired_previous_decision,
                             const base::Callback<void(content::CertificateRequestResultType)>& callback) override;
  void SelectClientCertificate(content::WebContents* web_contents,
                               net::SSLCertRequestInfo* cert_request_info,
                               std::unique_ptr<content::ClientCertificateDelegate> delegate) override;
#if 0
  void ResourceDispatcherHostCreated() override;
  bool CanCreateWindow(const GURL& opener_url,
                       const GURL& opener_top_level_frame_url,
                       const GURL& source_origin,
                       WindowContainerType container_type,
                       const std::string& frame_name,
                       const GURL& target_url,
                       const content::Referrer& referrer,
                       WindowOpenDisposition disposition,
                       const blink::WebWindowFeatures& features,
                       const std::vector<base::string16>& additional_features,
                       bool user_gesture,
                       bool opener_suppressed,
                       content::ResourceContext* context,
                       int render_process_id,
                       int opener_render_view_id,
                       int opener_render_frame_id,
                       bool* no_javascript_access) override;
#endif
  void GetAdditionalAllowedSchemesForFileSystem(std::vector<std::string>* schemes) override;

  // brightray::BrowserClient:
  brightray::BrowserMainParts* OverrideCreateBrowserMainParts(const content::MainFunctionParams&) override;
  void WebNotificationAllowed(int render_process_id, const base::Callback<void(bool, bool)>& callback) override;

 public:
  // Don't force renderer process to restart for once.
  static void SuppressRendererProcessRestartForOnce();

 private:
  void AddSandboxedRendererId(int process_id);
  void RemoveSandboxedRendererId(int process_id);
  bool IsRendererSandboxed(int process_id);
  bool ShouldCreateNewSiteInstance(content::BrowserContext* browser_context,
                                   content::SiteInstance* current_instance,
                                   const GURL& dest_url);

 private:
  Delegate* delegate_;
  std::map<int, int> pending_processes_;
  std::set<int> sandboxed_renderers_;
  base::Lock sandboxed_renderers_lock_;
  DISALLOW_COPY_AND_ASSIGN(MesonBrowserClient);
};
}
