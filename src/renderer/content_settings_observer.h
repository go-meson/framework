//-*-c++-*-
#pragma once

#include "base/compiler_specific.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/WebKit/public/web/WebContentSettingsClient.h"

namespace meson {
class ContentSettingsObserver : public content::RenderFrameObserver, public blink::WebContentSettingsClient {
 public:
  explicit ContentSettingsObserver(content::RenderFrame* render_frame);
  ~ContentSettingsObserver() override;

  // blink::WebContentSettingsClient implementation.
  bool allowDatabase(const blink::WebString& name, const blink::WebString& display_name, unsigned estimated_size) override;
  bool allowStorage(bool local) override;
  bool allowIndexedDB(const blink::WebString& name, const blink::WebSecurityOrigin& security_origin) override;

 private:
  // content::RenderFrameObserver implementation.
  void OnDestruct() override;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsObserver);
};
}
