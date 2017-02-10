#pragma once
#include "base/callback.h"
#include "content/public/renderer/browser_plugin_delegate.h"
#include "content/public/renderer/render_frame.h"

namespace gfx {
class Size;
}

namespace meson {

class GuestViewContainer : public content::BrowserPluginDelegate {
 public:
  typedef base::Callback<void(const gfx::Size&)> ResizeCallback;

  explicit GuestViewContainer(content::RenderFrame* render_frame);
  ~GuestViewContainer() override;

  static GuestViewContainer* FromID(int element_instance_id);

  void RegisterElementResizeCallback(const ResizeCallback& callback);

  // content::BrowserPluginDelegate:
  void SetElementInstanceID(int element_instance_id) final;
  void DidResizeElement(const gfx::Size& new_size) final;
  base::WeakPtr<BrowserPluginDelegate> GetWeakPtr() final;

 private:
  int element_instance_id_;

  ResizeCallback element_resize_callback_;

  base::WeakPtrFactory<GuestViewContainer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GuestViewContainer);
};
}
