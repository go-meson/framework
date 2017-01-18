#pragma once

#include "content/public/browser/browser_plugin_guest_delegate.h"
#include "content/public/browser/web_contents_observer.h"

namespace meson {
class MesonWebContentsBinding;

// A struct of parameters for SetSize(). The parameters are all declared as
// scoped pointers since they are all optional. Null pointers indicate that the
// parameter has not been provided, and the last used value should be used. Note
// that when |enable_auto_size| is true, providing |normal_size| is not
// meaningful. This is because the normal size of the guestview is overridden
// whenever autosizing occurs.
struct SetSizeParams {
  SetSizeParams() {}
  ~SetSizeParams() {}

  std::unique_ptr<bool> enable_auto_size;
  std::unique_ptr<gfx::Size> min_size;
  std::unique_ptr<gfx::Size> max_size;
  std::unique_ptr<gfx::Size> normal_size;
};

class WebViewGuestDelegate : public content::BrowserPluginGuestDelegate,
                             public content::WebContentsObserver {
 public:
  WebViewGuestDelegate();
  ~WebViewGuestDelegate() override;

  void Initialize(MesonWebContentsBinding* api_web_contents);

  // Called when the WebContents is going to be destroyed.
  void Destroy();

  // Used to toggle autosize mode for this GuestView, and set both the automatic
  // and normal sizes.
  void SetSize(const SetSizeParams& params);

 protected:
  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* navigation_handle) override;

  // content::BrowserPluginGuestDelegate:
  void DidAttach(int guest_proxy_routing_id) override final;
  content::WebContents* GetOwnerWebContents() const override final;
  void GuestSizeChanged(const gfx::Size& new_size) override final;
  void SetGuestHost(content::GuestHost* guest_host) override final;
  void WillAttach(content::WebContents* embedder_web_contents,
                  int element_instance_id,
                  bool is_full_page_plugin,
                  const base::Closure& completion_callback) override final;

 private:
  // This method is invoked when the contents auto-resized to give the container
  // an opportunity to match it if it wishes.
  //
  // This gives the derived class an opportunity to inform its container element
  // or perform other actions.
  void GuestSizeChangedDueToAutoSize(const gfx::Size& old_size, const gfx::Size& new_size);

  // Returns the default size of the guestview.
  gfx::Size GetDefaultSize() const;

  // The WebContents that attaches this guest view.
  content::WebContents* embedder_web_contents_;

  // The size of the container element.
  gfx::Size element_size_;

  // The size of the guest content. Note: In autosize mode, the container
  // element may not match the size of the guest.
  gfx::Size guest_size_;

  // A pointer to the guest_host.
  content::GuestHost* guest_host_;

  // Indicates whether autosize mode is enabled or not.
  bool auto_size_enabled_;

  // The maximum size constraints of the container element in autosize mode.
  gfx::Size max_auto_size_;

  // The minimum size constraints of the container element in autosize mode.
  gfx::Size min_auto_size_;

  // The size that will be used when autosize mode is disabled.
  gfx::Size normal_size_;

  // Whether the guest view is inside a plugin document.
  bool is_full_page_plugin_;

  MesonWebContentsBinding* api_web_contents_;

  DISALLOW_COPY_AND_ASSIGN(WebViewGuestDelegate);
};

}