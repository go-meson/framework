//-*-c++-*-
#pragma once
#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace meson {

class MesonResourceDispatcherHostDelegate : public content::ResourceDispatcherHostDelegate {
 public:
  MesonResourceDispatcherHostDelegate();

  // content::ResourceDispatcherHostDelegate:
  bool HandleExternalProtocol(const GURL& url, content::ResourceRequestInfo* info) override;

  content::ResourceDispatcherHostLoginDelegate* CreateLoginDelegate(net::AuthChallengeInfo* auth_info,
                                                                    net::URLRequest* request) override;
  std::unique_ptr<net::ClientCertStore> CreateClientCertStore(content::ResourceContext* resource_context) override;
};
}
