#include "browser/web_view_guest_delegate.h"

#include "api/web_contents_binding.h"
#include "content/public/browser/guest_host.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"

namespace meson {

namespace {

const int kDefaultWidth = 300;
const int kDefaultHeight = 300;

}  // namespace

WebViewGuestDelegate::WebViewGuestDelegate()
    : guest_host_(nullptr),
      auto_size_enabled_(false),
      is_full_page_plugin_(false),
      api_web_contents_(nullptr) {
  LOG(INFO) << __PRETTY_FUNCTION__;
}

WebViewGuestDelegate::~WebViewGuestDelegate() {
  LOG(INFO) << __PRETTY_FUNCTION__;
}

void WebViewGuestDelegate::Initialize(WebContentsBinding* api_web_contents) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  api_web_contents_ = api_web_contents;
  Observe(api_web_contents->GetWebContents());
}

void WebViewGuestDelegate::Destroy() {
  // Give the content module an opportunity to perform some cleanup.
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (guest_host_) {
    guest_host_->WillDestroy();
    guest_host_ = nullptr;
  }
}

void WebViewGuestDelegate::SetSize(const SetSizeParams& params) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  bool enable_auto_size = params.enable_auto_size ? *params.enable_auto_size : auto_size_enabled_;
  gfx::Size min_size = params.min_size ? *params.min_size : min_auto_size_;
  gfx::Size max_size = params.max_size ? *params.max_size : max_auto_size_;

  if (params.normal_size)
    normal_size_ = *params.normal_size;

  min_auto_size_ = min_size;
  min_auto_size_.SetToMin(max_size);
  max_auto_size_ = max_size;
  max_auto_size_.SetToMax(min_size);
  enable_auto_size &= !min_auto_size_.IsEmpty() && !max_auto_size_.IsEmpty();

  auto rvh = web_contents()->GetRenderViewHost();
  if (enable_auto_size) {
    // Autosize is being enabled.
    rvh->EnableAutoResize(min_auto_size_, max_auto_size_);
    normal_size_.SetSize(0, 0);
  } else {
    // Autosize is being disabled.
    // Use default width/height if missing from partially defined normal size.
    if (normal_size_.width() && !normal_size_.height())
      normal_size_.set_height(GetDefaultSize().height());
    if (!normal_size_.width() && normal_size_.height())
      normal_size_.set_width(GetDefaultSize().width());

    gfx::Size new_size;
    if (!normal_size_.IsEmpty()) {
      new_size = normal_size_;
    } else if (!guest_size_.IsEmpty()) {
      new_size = guest_size_;
    } else {
      new_size = GetDefaultSize();
    }

    if (auto_size_enabled_) {
      // Autosize was previously enabled.
      rvh->DisableAutoResize(new_size);
      GuestSizeChangedDueToAutoSize(guest_size_, new_size);
    } else {
      // Autosize was already disabled.
      guest_host_->SizeContents(new_size);
    }

    guest_size_ = new_size;
  }

  auto_size_enabled_ = enable_auto_size;
}

void WebViewGuestDelegate::DidFinishNavigation(content::NavigationHandle* navigation_handle) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (navigation_handle->HasCommitted() && !navigation_handle->IsErrorPage()) {
    auto is_main_frame = navigation_handle->IsInMainFrame();
    auto url = navigation_handle->GetURL();

    api_web_contents_->EmitEvent("load-commit", "url", url, "isMainFrame", is_main_frame);
  }
}

void WebViewGuestDelegate::DidAttach(int guest_proxy_routing_id) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  api_web_contents_->EmitEvent("did-attach");
}

content::WebContents* WebViewGuestDelegate::GetOwnerWebContents() const {
  LOG(INFO) << __PRETTY_FUNCTION__;
  return embedder_web_contents_;
}

void WebViewGuestDelegate::GuestSizeChanged(const gfx::Size& new_size) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (!auto_size_enabled_)
    return;
  GuestSizeChangedDueToAutoSize(guest_size_, new_size);
  guest_size_ = new_size;
}

void WebViewGuestDelegate::SetGuestHost(content::GuestHost* guest_host) {
  LOG(INFO) << __PRETTY_FUNCTION__ << (long long)(guest_host);
  guest_host_ = guest_host;
}

void WebViewGuestDelegate::WillAttach(content::WebContents* embedder_web_contents,
                                      int element_instance_id,
                                      bool is_full_page_plugin,
                                      const base::Closure& completion_callback) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  embedder_web_contents_ = embedder_web_contents;
  is_full_page_plugin_ = is_full_page_plugin;
  completion_callback.Run();
}

void WebViewGuestDelegate::GuestSizeChangedDueToAutoSize(const gfx::Size& old_size,
                                                         const gfx::Size& new_size) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  api_web_contents_->EmitEvent("size-changed",
                               "oldSize", old_size,
                               "newSize", new_size);
}

gfx::Size WebViewGuestDelegate::GetDefaultSize() const {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (is_full_page_plugin_) {
    // Full page plugins default to the size of the owner's viewport.
    return embedder_web_contents_->GetRenderWidgetHostView()->GetVisibleViewportSize();
  } else {
    return gfx::Size(kDefaultWidth, kDefaultHeight);
  }
}

bool WebViewGuestDelegate::CanBeEmbeddedInsideCrossProcessFrames() {
  return true;
}

content::RenderWidgetHost* WebViewGuestDelegate::GetOwnerRenderWidgetHost() {
  return embedder_web_contents_->GetRenderViewHost()->GetWidget();
}

content::SiteInstance* WebViewGuestDelegate::GetOwnerSiteInstance() {
  return embedder_web_contents_->GetSiteInstance();
}

}  // namespace atom
