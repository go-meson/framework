//-*-c++-*-
#pragma once

#include <map>
#include <vector>

#include "base/callback.h"
#include "base/id_map.h"
#include "content/public/browser/permission_manager.h"

namespace content {
class WebContents;
}

namespace meson {

class MesonPermissionManager : public content::PermissionManager {
 public:
  MesonPermissionManager();
  ~MesonPermissionManager() override;

  using StatusCallback = base::Callback<void(blink::mojom::PermissionStatus)>;
  using StatusesCallback = base::Callback<void(const std::vector<blink::mojom::PermissionStatus>&)>;
  using RequestHandler = base::Callback<void(content::WebContents*, content::PermissionType, const StatusCallback&)>;

  // Handler to dispatch permission requests in JS.
  void SetPermissionRequestHandler(const RequestHandler& handler);

  // content::PermissionManager:
  int RequestPermission(content::PermissionType permission,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& requesting_origin,
                        bool user_gesture,
                        const base::Callback<void(blink::mojom::PermissionStatus)>& callback) override;
  int RequestPermissions(const std::vector<content::PermissionType>& permissions,
                         content::RenderFrameHost* render_frame_host,
                         const GURL& requesting_origin,
                         bool user_gesture,
                         const base::Callback<void(const std::vector<blink::mojom::PermissionStatus>&)>& callback) override;

 protected:
  void OnPermissionResponse(int request_id, int permission_id, blink::mojom::PermissionStatus status);
  // content::PermissionManager:
  void CancelPermissionRequest(int request_id) override;
  void ResetPermission(content::PermissionType permission, const GURL& requesting_origin, const GURL& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatus(content::PermissionType permission, const GURL& requesting_origin, const GURL& embedding_origin) override;
  void RegisterPermissionUsage(content::PermissionType permission, const GURL& requesting_origin, const GURL& embedding_origin) override;
  int SubscribePermissionStatusChange(content::PermissionType permission,
                                      const GURL& requesting_origin,
                                      const GURL& embedding_origin,
                                      const base::Callback<void(blink::mojom::PermissionStatus)>& callback) override;
  void UnsubscribePermissionStatusChange(int subscription_id) override;

 private:
  class PendingRequest;
  using PendingRequestMap = IDMap<PendingRequest, IDMapOwnPointer>;

  RequestHandler request_handler_;

  PendingRequestMap pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(MesonPermissionManager);
};
}
