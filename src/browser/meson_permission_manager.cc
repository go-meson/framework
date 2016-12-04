#include "browser/meson_permission_manager.h"

#include <vector>

#include "browser/web_contents_preferences.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

namespace meson {

namespace {

bool WebContentsDestroyed(int process_id) {
  auto contents =
      WebContentsPreferences::GetWebContentsFromProcessID(process_id);
  if (!contents)
    return true;
  return contents->IsBeingDestroyed();
}

}

MesonPermissionManager::MesonPermissionManager()
    : request_id_(0) {
}

MesonPermissionManager::~MesonPermissionManager() {
}

void MesonPermissionManager::SetPermissionRequestHandler(
    const RequestHandler& handler) {
  if (handler.is_null() && !pending_requests_.empty()) {
    for (const auto& request : pending_requests_) {
      if (!WebContentsDestroyed(request.second.render_process_id))
        request.second.callback.Run(blink::mojom::PermissionStatus::DENIED);
    }
    pending_requests_.clear();
  }
  request_handler_ = handler;
}

int MesonPermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const ResponseCallback& response_callback) {
  int process_id = render_frame_host->GetProcess()->GetID();

  if (permission == content::PermissionType::MIDI_SYSEX) {
    content::ChildProcessSecurityPolicy::GetInstance()->
        GrantSendMidiSysExMessage(process_id);
  }

  if (!request_handler_.is_null()) {
    auto web_contents =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    ++request_id_;
    auto callback = base::Bind(&MesonPermissionManager::OnPermissionResponse,
                               base::Unretained(this),
                               request_id_,
                               requesting_origin,
                               response_callback);
    pending_requests_[request_id_] = { process_id, callback };
    request_handler_.Run(web_contents, permission, callback);
    return request_id_;
  }

  response_callback.Run(blink::mojom::PermissionStatus::GRANTED);
  return kNoPendingOperation;
}

int MesonPermissionManager::RequestPermissions(
    const std::vector<content::PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const base::Callback<void(
    const std::vector<blink::mojom::PermissionStatus>&)>& callback) {
  // FIXME(zcbenz): Just ignore multiple permissions request for now.
  std::vector<blink::mojom::PermissionStatus> permissionStatuses;
  for (auto permission : permissions) {
    if (permission == content::PermissionType::MIDI_SYSEX) {
      content::ChildProcessSecurityPolicy::GetInstance()->
          GrantSendMidiSysExMessage(render_frame_host->GetProcess()->GetID());
    }
    permissionStatuses.push_back(blink::mojom::PermissionStatus::GRANTED);
  }
  callback.Run(permissionStatuses);
  return kNoPendingOperation;
}

void MesonPermissionManager::OnPermissionResponse(
    int request_id,
    const GURL& origin,
    const ResponseCallback& callback,
    blink::mojom::PermissionStatus status) {
  auto request = pending_requests_.find(request_id);
  if (request != pending_requests_.end()) {
    if (!WebContentsDestroyed(request->second.render_process_id))
      callback.Run(status);
    pending_requests_.erase(request);
  }
}

void MesonPermissionManager::CancelPermissionRequest(int request_id) {
  auto request = pending_requests_.find(request_id);
  if (request != pending_requests_.end()) {
    if (!WebContentsDestroyed(request->second.render_process_id))
      request->second.callback.Run(blink::mojom::PermissionStatus::DENIED);
    pending_requests_.erase(request);
  }
}

void MesonPermissionManager::ResetPermission(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

blink::mojom::PermissionStatus MesonPermissionManager::GetPermissionStatus(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  return blink::mojom::PermissionStatus::GRANTED;
}

void MesonPermissionManager::RegisterPermissionUsage(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

int MesonPermissionManager::SubscribePermissionStatusChange(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    const ResponseCallback& callback) {
  return -1;
}

void MesonPermissionManager::UnsubscribePermissionStatusChange(
    int subscription_id) {
}

}
