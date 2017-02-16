#include "browser/meson_resource_dispatcher_host_delegate.h"

#include "browser/login_handler.h"
#include "browser/web_contents_permission_helper.h"
#include "common/platform_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"
#include "net/ssl/client_cert_store.h"
#include "url/gurl.h"

#if defined(USE_NSS_CERTS)
#include "net/ssl/client_cert_store_nss.h"
#elif defined(OS_WIN)
#include "net/ssl/client_cert_store_win.h"
#elif defined(OS_MACOSX)
#include "net/ssl/client_cert_store_mac.h"
#endif

using content::BrowserThread;

namespace meson {

namespace {

void OnOpenExternal(const GURL& escaped_url,
                    bool allowed) {
  if (allowed)
    platform_util::OpenExternal(
#if defined(OS_WIN)
        base::UTF8ToUTF16(escaped_url.spec()),
#else
        escaped_url,
#endif
        true);
}

void HandleExternalProtocolInUI(
    const GURL& url,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    bool has_user_gesture) {
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return;

  auto permission_helper = WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper)
    return;

  GURL escaped_url(net::EscapeExternalHandlerValue(url.spec()));
  auto callback = base::Bind(&OnOpenExternal, escaped_url);
  permission_helper->RequestOpenExternalPermission(callback, has_user_gesture);
}

}  // namespace

MesonResourceDispatcherHostDelegate::MesonResourceDispatcherHostDelegate() {
}

  bool MesonResourceDispatcherHostDelegate::HandleExternalProtocol(const GURL& url, content::ResourceRequestInfo* info) {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&HandleExternalProtocolInUI,
                                     url,
                                     info->GetWebContentsGetterForRequest(),
                                     info->HasUserGesture()));
  return true;
}

content::ResourceDispatcherHostLoginDelegate* MesonResourceDispatcherHostDelegate::CreateLoginDelegate(net::AuthChallengeInfo* auth_info, net::URLRequest* request) {
  return new LoginHandler(auth_info, request);
}

std::unique_ptr<net::ClientCertStore> MesonResourceDispatcherHostDelegate::CreateClientCertStore(content::ResourceContext* resource_context) {
#if defined(USE_NSS_CERTS)
  return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreNSS(net::ClientCertStoreNSS::PasswordDelegateFactory()));
#elif defined(OS_WIN)
  return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreWin());
#elif defined(OS_MACOSX)
  return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreMac());
#elif defined(USE_OPENSSL)
  return std::unique_ptr<net::ClientCertStore>();
#endif
}
}
