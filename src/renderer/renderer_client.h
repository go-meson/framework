//-*-c++-*-
#pragma once

#include <memory>
#include "content/public/renderer/content_renderer_client.h"
#include "renderer/extensions/local_source_map.h"

namespace meson {
class PreferencesManager;
class MesonRendererClient : public content::ContentRendererClient {
 public:
  MesonRendererClient();
  virtual ~MesonRendererClient();

  static MesonRendererClient* Get();

 private:
  // content::ContentRendererClient:
  void RenderThreadStarted() override;
  void RenderViewCreated(content::RenderView*) override;
  void RenderFrameCreated(content::RenderFrame*) override;
  bool OverrideCreatePlugin(content::RenderFrame* render_frame,
                            blink::WebLocalFrame* frame,
                            const blink::WebPluginParams& params,
                            blink::WebPlugin** plugin) override;
  bool ShouldFork(blink::WebLocalFrame* frame,
                  const GURL& url,
                  const std::string& http_method,
                  bool is_initial_navigation,
                  bool is_server_redirect,
                  bool* send_referrer) override;
  content::BrowserPluginDelegate* CreateBrowserPluginDelegate(content::RenderFrame* render_frame,
                                                              const std::string& mime_type,
                                                              const GURL& original_url) override;
#if 0
  //TODO:
  blink::WebSpeechSynthesizer* OverrideSpeechSynthesizer(blink::WebSpeechSynthesizerClient* client) override;
#endif
#if 0
  void AddSupportedKeySystems(std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems) override;
#endif

 public:
  bool OnMessageReceived(const IPC::Message& message);
  void DidClearWindowObject(content::RenderFrame* render_frame);
  void DidCreateScriptContext(v8::Handle<v8::Context> context, content::RenderFrame* render_frame);
  void WillReleaseScriptContext(v8::Handle<v8::Context> context, content::RenderFrame* render_frame);

 private:
  std::unique_ptr<PreferencesManager> preferences_manager_;
  extensions::LocalSourceMap source_map_;
  DISALLOW_COPY_AND_ASSIGN(MesonRendererClient);
};
}
