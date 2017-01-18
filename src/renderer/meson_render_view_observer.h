//-*-c++-*-
#pragma once

#include "base/strings/string16.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/WebKit/public/web/WebFrame.h"

namespace base {
class ListValue;
}

namespace meson {

class MesonRendererClient;

class MesonRenderViewObserver : public content::RenderViewObserver {
 public:
  explicit MesonRenderViewObserver(content::RenderView* render_view, MesonRendererClient* renderer_client);

 protected:
  virtual ~MesonRenderViewObserver();

  virtual void EmitIPCEvent(blink::WebFrame* frame,
                            const base::string16& channel,
                            const base::ListValue& args);

 private:
  // content::RenderViewObserver implementation.
  void DidCreateDocumentElement(blink::WebLocalFrame* frame) override;
  void DraggableRegionsChanged(blink::WebFrame* frame) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnDestruct() override;

  void OnBrowserMessage(bool send_to_all, const base::string16& channel, const base::ListValue& args);

  // Whether the document object has been created.
  bool document_created_;

  DISALLOW_COPY_AND_ASSIGN(MesonRenderViewObserver);
};
}
