#include "renderer/meson_render_frame_observer.h"

#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebDraggableRegion.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"

#include "common/switches.h"
#include "api/api_messages.h"
#include "renderer/renderer_client.h"
#include "renderer/extensions/web_view_bindings.h"
#include "renderer/extensions/remote_bindings.h"

using namespace content;

namespace meson {

std::vector<MesonRenderFrameObserver*> MesonRenderFrameObserver::s_instances;

// static
MesonRenderFrameObserver* MesonRenderFrameObserver::FromRenderFrame(RenderFrame* render_frame) {
  for (size_t i = 0; i < s_instances.size(); ++i) {
    if (s_instances[i]->render_frame() == render_frame) {
      return s_instances[i];
    }
  }
  return nullptr;
}

MesonRenderFrameObserver::MesonRenderFrameObserver(RenderFrame* render_frame, MesonRendererClient* renderer_client)
    : RenderFrameObserver(render_frame), world_id_(-1), renderer_client_(renderer_client) {
  LOG(INFO) << "RENDER FRAME CREATED " << render_frame << " : " << this->render_frame();;
  s_instances.push_back(this);
}

MesonRenderFrameObserver::~MesonRenderFrameObserver() {
  for (size_t i = 0; i < s_instances.size(); ++i) {
    if (s_instances[i] == this) {
      s_instances.erase(s_instances.begin() + i);
      break;
    }
  }
}

/******************************************************************************/
/* RENDERFRAMEOBSERVER IMPLEMENTATION */
/******************************************************************************/

void MesonRenderFrameObserver::DidClearWindowObject() {
  LOG(INFO) << __PRETTY_FUNCTION__;
  renderer_client_->DidClearWindowObject(render_frame());
}
void MesonRenderFrameObserver::DidCreateScriptContext(v8::Handle<v8::Context> context, int extension_group, int world_id) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << world_id;
  if (world_id_ != -1 && world_id_ != world_id)
    return;
  world_id_ = world_id;
  renderer_client_->DidCreateScriptContext(context, render_frame());
}
void MesonRenderFrameObserver::WillReleaseScriptContext(v8::Local<v8::Context> context, int world_id) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (world_id_ != world_id)
    return;

  renderer_client_->WillReleaseScriptContext(context, render_frame());
}
void MesonRenderFrameObserver::OnDestruct() {
  LOG(INFO) << __PRETTY_FUNCTION__;
  delete this;
}

bool MesonRenderFrameObserver::OnMessageReceived(const IPC::Message& message) {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MesonRenderFrameObserver, message)
    IPC_MESSAGE_HANDLER(MesonFrameMsg_WebViewEmit,
                        WebViewEmit)
    IPC_MESSAGE_HANDLER(MesonFrameMsg_RemoteDispatch,
                        RemoteDispatch)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

/******************************************************************************/
/* WEBVIEW MESSAGE HANDLING */
/******************************************************************************/
void MesonRenderFrameObserver::AddWebViewBindings(extensions::WebViewBindings* bindings) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  /* Does not own bindings just stores a pointer to it. */
  web_view_bindings_.push_back(bindings);
}

void MesonRenderFrameObserver::RemoveWebViewBindings(extensions::WebViewBindings* bindings) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  for (size_t i = 0; i < web_view_bindings_.size(); ++i) {
    if (web_view_bindings_[i] == bindings) {
      web_view_bindings_.erase(web_view_bindings_.begin() + i);
      break;
    }
  }
}

void MesonRenderFrameObserver::WebViewEmit(int guest_instance_id,
                                           const std::string type,
                                           const base::DictionaryValue& event) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  for (size_t i = 0; i < web_view_bindings_.size(); ++i) {
    web_view_bindings_[i]->AttemptEmitEvent(guest_instance_id, type, event);
  }
}

/******************************************************************************/
/* REMOTE MESSAGE HANDLING */
/******************************************************************************/
void MesonRenderFrameObserver::AddRemoteBindings(extensions::RemoteBindings* bindings) {
  /* Does not own bindings just stores a pointer to it. */
  remote_bindings_.push_back(bindings);
}

void MesonRenderFrameObserver::RemoveRemoteBindings(extensions::RemoteBindings* bindings) {
  for (size_t i = 0; i < remote_bindings_.size(); ++i) {
    if (remote_bindings_[i] == bindings) {
      remote_bindings_.erase(remote_bindings_.begin() + i);
      break;
    }
  }
}

void MesonRenderFrameObserver::RemoteDispatch(const base::DictionaryValue& message) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  for (size_t i = 0; i < remote_bindings_.size(); ++i) {
    /* TODO(spolu): THERE SHOULD BE ONLY ONE */
    remote_bindings_[i]->DispatchMessage(message);
  }
}

}  // namespace thrust_shell
