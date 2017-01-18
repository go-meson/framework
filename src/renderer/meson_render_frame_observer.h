#pragma once

#include <vector>
#include "base/values.h"
#include "content/public/renderer/render_frame_observer.h"

namespace blink {
class WebFrame;
}

namespace context {
class RenderFrame;
}

namespace extensions {
class WebViewBindings;
class RemoteBindings;
}

namespace meson {
class MesonRendererClient;
class MesonRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit MesonRenderFrameObserver(content::RenderFrame* render_frame, MesonRendererClient* renderer_client);
  virtual ~MesonRenderFrameObserver();

  static MesonRenderFrameObserver* FromRenderFrame(content::RenderFrame* render_frame);

  void DidClearWindowObject() override;
  void DidCreateScriptContext(v8::Handle<v8::Context> context, int extension_group, int world_id) override;
  void WillReleaseScriptContext(v8::Local<v8::Context> context, int world_id) override;
  void OnDestruct() override;
  bool OnMessageReceived(const IPC::Message& message) override;

  // WEBVIEW MESSAGE HANDLING
  void AddWebViewBindings(extensions::WebViewBindings* bindings);
  void RemoveWebViewBindings(extensions::WebViewBindings* bindings);
  void WebViewEmit(int guest_instance_id, const std::string type, const base::DictionaryValue& event);

  // REMOTE MESSAGE HANDLING
  void AddRemoteBindings(extensions::RemoteBindings* bindings);
  void RemoveRemoteBindings(extensions::RemoteBindings* bindings);
  void RemoteDispatch(const base::DictionaryValue& message);

 private:
  // A static container of all the instances.
  static std::vector<MesonRenderFrameObserver*> s_instances;

  int world_id_;
  MesonRendererClient* renderer_client_;
  std::vector<extensions::WebViewBindings*> web_view_bindings_;
  std::vector<extensions::RemoteBindings*> remote_bindings_;

  DISALLOW_COPY_AND_ASSIGN(MesonRenderFrameObserver);
};
}
