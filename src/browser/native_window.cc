#include "browser/native_window.h"

#include <string>
#include <utility>
#include <vector>
#include <tuple>

#include "browser/session/meson_browser_context.h"
#include "browser/browser_main_parts.h"
#include "browser/browser.h"
#include "browser/unresponsive_suppressor.h"
#include "common/draggable_region.h"
#include "browser/window_list.h"
#include "api/api_messages.h"
#include "common/options_switches.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "components/prefs/pref_service.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/content_switches.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gl/gpu_switching_manager.h"

#if defined(OS_LINUX) || defined(OS_WIN)
#include "content/public/common/renderer_preferences.h"
#include "ui/gfx/font_render_params.h"
#endif

#include "browser/web_view_manager.h"
#include "api/web_contents_binding.h"
#include "api/api.h"
#include "browser/web_view_guest_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "third_party/WebKit/public/web/WebFindOptions.h"
#include "browser/web_contents_preferences.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(meson::NativeWindowRelay);

namespace meson {
#if 0
//TODO:
struct GuestInstance {
  int elementInstanceId;
  scoped_refptr<MesonWebContentsBinding> embedder;
  scoped_refptr<MesonWebContentsBinding> binding;
  base::DictionaryValue attachParams;
  GuestInstance(scoped_refptr<MesonWebContentsBinding> e, scoped_refptr<MesonWebContentsBinding> b)
      : elementInstanceId(-1), embedder(e), binding(b) {}
};
std::map<int, GuestInstance> guestInstances;
#endif

NativeWindow::NativeWindow(brightray::InspectableWebContents* inspectable_web_contents, const base::DictionaryValue& options, NativeWindow* parent)
    : content::WebContentsObserver(inspectable_web_contents->GetWebContents()),
      has_frame_(true),
      transparent_(false),
      enable_larger_than_screen_(false),
      is_closed_(false),
      sheet_offset_x_(0.0),
      sheet_offset_y_(0.0),
      aspect_ratio_(0.0),
      parent_(parent),
      is_modal_(false),
      inspectable_web_contents_(inspectable_web_contents),
      weak_factory_(this) {
  options.GetBoolean(options::kFrame, &has_frame_);
  options.GetBoolean(options::kTransparent, &transparent_);
  options.GetBoolean(options::kEnableLargerThanScreen, &enable_larger_than_screen_);

  if (parent)
    options.GetBoolean("modal", &is_modal_);

#if defined(OS_LINUX) || defined(OS_WIN)
  auto* prefs = web_contents()->GetMutableRendererPrefs();

  // Update font settings.
  CR_DEFINE_STATIC_LOCAL(const gfx::FontRenderParams, params, (gfx::GetFontRenderParams(gfx::FontRenderParamsQuery(), nullptr)));
  prefs->should_antialias_text = params.antialiasing;
  prefs->use_subpixel_positioning = params.subpixel_positioning;
  prefs->hinting = params.hinting;
  prefs->use_autohinter = params.autohinter;
  prefs->use_bitmaps = params.use_bitmaps;
  prefs->subpixel_rendering = params.subpixel_rendering;
#endif

  ui::GpuSwitchingManager::SetTransparent(transparent_);

  WindowList::AddWindow(this);
}

NativeWindow::~NativeWindow() {
  LOG(INFO) << __PRETTY_FUNCTION__;
  NotifyWindowClosed();
}

NativeWindow* NativeWindow::FromWebContents(content::WebContents* web_contents) {
  auto& window_list = *WindowList::GetInstance();
  for (auto* window : window_list) {
    if (window->web_contents() == web_contents)
      return window;
  }
  return nullptr;
}

void NativeWindow::InitFromOptions(const base::DictionaryValue& options) {
  // Setup window from options.
  int x = -1, y = -1;
  bool center;
  if (options.GetInteger(options::kX, &x) && options.GetInteger(options::kY, &y)) {
    SetPosition(gfx::Point(x, y));
  } else if (options.GetBoolean(options::kCenter, &center) && center) {
    Center();
  }

  extensions::SizeConstraints size_constraints(GetContentSizeConstraints());
  int min_height = 0, min_width = 0;
  if (options.GetInteger(options::kMinHeight, &min_height) |
      options.GetInteger(options::kMinWidth, &min_width)) {
    size_constraints.set_minimum_size(gfx::Size(min_width, min_height));
  }
  int max_height = INT_MAX, max_width = INT_MAX;
  if (options.GetInteger(options::kMaxHeight, &max_height) |
      options.GetInteger(options::kMaxWidth, &max_width)) {
    size_constraints.set_maximum_size(gfx::Size(max_width, max_height));
  }
  bool use_content_size = false;
  options.GetBoolean(options::kUseContentSize, &use_content_size);
  if (use_content_size) {
    SetContentSizeConstraints(size_constraints);
  } else {
    SetSizeConstraints(size_constraints);
  }
#if defined(USE_X11)
  bool resizable;
  if (options.GetBoolean(options::kResizable, &resizable)) {
    SetResizable(resizable);
  }
#endif
#if defined(OS_WIN) || defined(USE_X11)
  bool closable;
  if (options.GetBoolean(options::kClosable, &closable)) {
    SetClosable(closable);
  }
#endif
  bool movable;
  if (options.GetBoolean(options::kMovable, &movable)) {
    SetMovable(movable);
  }
  bool has_shadow;
  if (options.GetBoolean(options::kHasShadow, &has_shadow)) {
    SetHasShadow(has_shadow);
  }
  bool top;
  if (options.GetBoolean(options::kAlwaysOnTop, &top) && top) {
    SetAlwaysOnTop(true);
  }
  bool fullscreenable = true;
  bool fullscreen = false;
  if (options.GetBoolean(options::kFullscreen, &fullscreen) && !fullscreen) {
// Disable fullscreen button if 'fullscreen' is specified to false.
#if defined(OS_MACOSX)
    fullscreenable = false;
#endif
  }
  // Overriden by 'fullscreenable'.
  options.GetBoolean(options::kFullScreenable, &fullscreenable);
  SetFullScreenable(fullscreenable);
  if (fullscreen) {
    SetFullScreen(true);
  }
  bool skip;
  if (options.GetBoolean(options::kSkipTaskbar, &skip)) {
    SetSkipTaskbar(skip);
  }
  bool kiosk;
  if (options.GetBoolean(options::kKiosk, &kiosk) && kiosk) {
    SetKiosk(kiosk);
  }
  std::string color;
  if (options.GetString(options::kBackgroundColor, &color)) {
    SetBackgroundColor(color);
  } else if (!transparent()) {
    SetBackgroundColor("#FFFF");
  }
  std::string title(Browser::Get()->GetName());
  options.GetString(options::kTitle, &title);
  SetTitle(title);

  // Then show it.
  bool show = true;
  options.GetBoolean(options::kShow, &show);
  if (show)
    Show();
}

void NativeWindow::SetSize(const gfx::Size& size, bool animate) {
  SetBounds(gfx::Rect(GetPosition(), size), animate);
}

gfx::Size NativeWindow::GetSize() {
  return GetBounds().size();
}

void NativeWindow::SetPosition(const gfx::Point& position, bool animate) {
  SetBounds(gfx::Rect(position, GetSize()), animate);
}

gfx::Point NativeWindow::GetPosition() {
  return GetBounds().origin();
}

void NativeWindow::SetContentSize(const gfx::Size& size, bool animate) {
  SetSize(ContentBoundsToWindowBounds(gfx::Rect(size)).size(), animate);
}

gfx::Size NativeWindow::GetContentSize() {
  return GetContentBounds().size();
}

void NativeWindow::SetContentBounds(const gfx::Rect& bounds, bool animate) {
  SetBounds(ContentBoundsToWindowBounds(bounds), animate);
}

gfx::Rect NativeWindow::GetContentBounds() {
  return WindowBoundsToContentBounds(GetBounds());
}

void NativeWindow::SetSizeConstraints(const extensions::SizeConstraints& window_constraints) {
  extensions::SizeConstraints content_constraints(GetContentSizeConstraints());
  if (window_constraints.HasMaximumSize()) {
    gfx::Rect max_bounds = WindowBoundsToContentBounds(
        gfx::Rect(window_constraints.GetMaximumSize()));
    content_constraints.set_maximum_size(max_bounds.size());
  }
  if (window_constraints.HasMinimumSize()) {
    gfx::Rect min_bounds = WindowBoundsToContentBounds(
        gfx::Rect(window_constraints.GetMinimumSize()));
    content_constraints.set_minimum_size(min_bounds.size());
  }
  SetContentSizeConstraints(content_constraints);
}

extensions::SizeConstraints NativeWindow::GetSizeConstraints() {
  extensions::SizeConstraints content_constraints = GetContentSizeConstraints();
  extensions::SizeConstraints window_constraints;
  if (content_constraints.HasMaximumSize()) {
    gfx::Rect max_bounds = ContentBoundsToWindowBounds(
        gfx::Rect(content_constraints.GetMaximumSize()));
    window_constraints.set_maximum_size(max_bounds.size());
  }
  if (content_constraints.HasMinimumSize()) {
    gfx::Rect min_bounds = ContentBoundsToWindowBounds(
        gfx::Rect(content_constraints.GetMinimumSize()));
    window_constraints.set_minimum_size(min_bounds.size());
  }
  return window_constraints;
}

void NativeWindow::SetContentSizeConstraints(const extensions::SizeConstraints& size_constraints) {
  size_constraints_ = size_constraints;
}

extensions::SizeConstraints NativeWindow::GetContentSizeConstraints() {
  return size_constraints_;
}

void NativeWindow::SetMinimumSize(const gfx::Size& size) {
  extensions::SizeConstraints size_constraints;
  size_constraints.set_minimum_size(size);
  SetSizeConstraints(size_constraints);
}

gfx::Size NativeWindow::GetMinimumSize() {
  return GetSizeConstraints().GetMinimumSize();
}

void NativeWindow::SetMaximumSize(const gfx::Size& size) {
  extensions::SizeConstraints size_constraints;
  size_constraints.set_maximum_size(size);
  SetSizeConstraints(size_constraints);
}

gfx::Size NativeWindow::GetMaximumSize() {
  return GetSizeConstraints().GetMaximumSize();
}

void NativeWindow::SetSheetOffset(const double offsetX, const double offsetY) {
  sheet_offset_x_ = offsetX;
  sheet_offset_y_ = offsetY;
}

double NativeWindow::GetSheetOffsetX() {
  return sheet_offset_x_;
}

double NativeWindow::GetSheetOffsetY() {
  return sheet_offset_y_;
}

void NativeWindow::SetRepresentedFilename(const std::string& filename) {}

std::string NativeWindow::GetRepresentedFilename() {
  return "";
}

void NativeWindow::SetDocumentEdited(bool edited) {
}

bool NativeWindow::IsDocumentEdited() {
  return false;
}

void NativeWindow::SetFocusable(bool focusable) {}
void NativeWindow::SetMenu(MesonMenuModel* menu) {}
void NativeWindow::SetParentWindow(NativeWindow* parent) {
  parent_ = parent;
}

void NativeWindow::FocusOnWebView() {
  web_contents()->GetRenderViewHost()->GetWidget()->Focus();
}

void NativeWindow::BlurWebView() {
  web_contents()->GetRenderViewHost()->GetWidget()->Blur();
}

bool NativeWindow::IsWebViewFocused() {
  auto host_view = web_contents()->GetRenderViewHost()->GetWidget()->GetView();
  return host_view && host_view->HasFocus();
}

void NativeWindow::SetAutoHideMenuBar(bool auto_hide) {}
bool NativeWindow::IsMenuBarAutoHide() {
  return false;
}
void NativeWindow::SetMenuBarVisibility(bool visible) {}
bool NativeWindow::IsMenuBarVisible() {
  return true;
}

double NativeWindow::GetAspectRatio() {
  return aspect_ratio_;
}
gfx::Size NativeWindow::GetAspectRatioExtraSize() {
  return aspect_ratio_extraSize_;
}
void NativeWindow::SetAspectRatio(double aspect_ratio,
                                  const gfx::Size& extra_size) {
  aspect_ratio_ = aspect_ratio;
  aspect_ratio_extraSize_ = extra_size;
}

void NativeWindow::RequestToClosePage() {
  bool prevent_default = false;
  for (NativeWindowObserver& observer : observers_)
    observer.WillCloseWindow(&prevent_default);
  if (prevent_default) {
    WindowList::WindowCloseCancelled(this);
    return;
  }

  // Assume the window is not responding if it doesn't cancel the close and is
  // not closed in 5s, in this way we can quickly show the unresponsive
  // dialog when the window is busy executing some script withouth waiting for
  // the unresponsive timeout.
  if (window_unresposive_closure_.IsCancelled())
    ScheduleUnresponsiveEvent(5000);

  if (!web_contents())
    // Already closed by renderer
    return;

  if (web_contents()->NeedToFireBeforeUnload()) {
    web_contents()->DispatchBeforeUnload();
  } else {
    web_contents()->Close();
  }
}

void NativeWindow::CloseContents(content::WebContents* source) {
  if (!inspectable_web_contents_)
    return;

  inspectable_web_contents_->GetView()->SetDelegate(nullptr);
  inspectable_web_contents_ = nullptr;
  Observe(nullptr);

  for (NativeWindowObserver& observer : observers_)
    observer.WillDestroyNativeObject();

  
  // When the web contents is gone, close the window immediately, but the
  // memory will not be freed until you call delete.
  // In this way, it would be safe to manage windows via smart pointers. If you
  // want to free memory when the window is closed, you can do deleting by
  // overriding the OnWindowClosed method in the observer.
  CloseImmediately();

  // Do not sent "unresponsive" event after window is closed.
  window_unresposive_closure_.Cancel();
}

void NativeWindow::RendererUnresponsive(content::WebContents* source) {
  // Schedule the unresponsive shortly later, since we may receive the
  // responsive event soon. This could happen after the whole application had
  // blocked for a while.
  // Also notice that when closing this event would be ignored because we have
  // explicitly started a close timeout counter. This is on purpose because we
  // don't want the unresponsive event to be sent too early when user is closing
  // the window.
  ScheduleUnresponsiveEvent(50);
}

void NativeWindow::RendererResponsive(content::WebContents* source) {
  window_unresposive_closure_.Cancel();
  for (NativeWindowObserver& observer : observers_)
    observer.OnRendererResponsive();
}

void NativeWindow::NotifyWindowClosed() {
  if (is_closed_)
    return;

  WindowList::RemoveWindow(this);

  is_closed_ = true;
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowClosed();
}

void NativeWindow::NotifyWindowBlur() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowBlur();
}

void NativeWindow::NotifyWindowFocus() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowFocus();
}

void NativeWindow::NotifyWindowShow() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowShow();
}

void NativeWindow::NotifyWindowHide() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowHide();
}

void NativeWindow::NotifyWindowMaximize() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMaximize();
}

void NativeWindow::NotifyWindowUnmaximize() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowUnmaximize();
}

void NativeWindow::NotifyWindowMinimize() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMinimize();
}

void NativeWindow::NotifyWindowRestore() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowRestore();
}

void NativeWindow::NotifyWindowResize() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowResize();
}

void NativeWindow::NotifyWindowMove() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMove();
}

void NativeWindow::NotifyWindowMoved() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMoved();
}

void NativeWindow::NotifyWindowEnterFullScreen() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowEnterFullScreen();
}

void NativeWindow::NotifyWindowScrollTouchBegin() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowScrollTouchBegin();
}

void NativeWindow::NotifyWindowScrollTouchEnd() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowScrollTouchEnd();
}

void NativeWindow::NotifyWindowScrollTouchEdge() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowScrollTouchEdge();
}

void NativeWindow::NotifyWindowSwipe(const std::string& direction) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowSwipe(direction);
}

void NativeWindow::NotifyWindowLeaveFullScreen() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowLeaveFullScreen();
}

void NativeWindow::NotifyWindowEnterHtmlFullScreen() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowEnterHtmlFullScreen();
}

void NativeWindow::NotifyWindowLeaveHtmlFullScreen() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowLeaveHtmlFullScreen();
}

void NativeWindow::NotifyWindowExecuteWindowsCommand(const std::string& command) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnExecuteWindowsCommand(command);
}

#if defined(OS_WIN)
void NativeWindow::NotifyWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) {
  for (NativeWindowObserver& observer : observers_)
    observer.OnWindowMessage(message, w_param, l_param);
}
#endif

std::unique_ptr<SkRegion> NativeWindow::DraggableRegionsToSkRegion(const std::vector<DraggableRegion>& regions) {
  std::unique_ptr<SkRegion> sk_region(new SkRegion);
  for (const DraggableRegion& region : regions) {
    sk_region->op(region.bounds.x(), region.bounds.y(), region.bounds.right(), region.bounds.bottom(),
                  region.draggable ? SkRegion::kUnion_Op : SkRegion::kDifference_Op);
  }
  return sk_region;
}

void NativeWindow::RenderViewCreated(content::RenderViewHost* render_view_host) {
  if (!transparent_)
    return;

  auto* impl = content::RenderWidgetHostImpl::FromID(
      render_view_host->GetProcess()->GetID(),
      render_view_host->GetRoutingID());
  if (impl)
    impl->SetBackgroundOpaque(false);
}

void NativeWindow::BeforeUnloadDialogCancelled() {
  WindowList::WindowCloseCancelled(this);

  window_unresposive_closure_.Cancel();
}

void NativeWindow::DidFirstVisuallyNonEmptyPaint() {
  if (IsVisible())
    return;

  // When there is a non-empty first paint, resize the RenderWidget to force
  // Chromium to draw.
  const auto view = web_contents()->GetRenderWidgetHostView();
  view->Show();
  view->SetSize(GetContentSize());

  // Emit the ReadyToShow event in next tick in case of pending drawing work.
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, base::Bind(&NativeWindow::NotifyReadyToShow, GetWeakPtr()));
}

bool NativeWindow::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(NativeWindow, message)
    IPC_MESSAGE_HANDLER(MesonViewHostMsg_UpdateDraggableRegions, UpdateDraggableRegions)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}
bool NativeWindow::OnMessageReceived(const IPC::Message& message, content::RenderFrameHost* render_frame_host) {
  bool handled = true;

  content::WebContents* wc = web_contents();
  WebViewManager* webviewManager = WebViewManager::GetWebViewManager(wc);
  if (webviewManager) {
    IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(NativeWindow, message, wc)
      IPC_MESSAGE_UNHANDLED(handled = false)
      IPC_MESSAGE_HANDLER(MesonFrameHostMsg_CreateWebViewGuest, OnCreateWebViewGuest)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_AttachWindowGuest, webviewManager, WebViewManager::OnAttachWindowGuest)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_DestroyWebViewGuest, webviewManager, WebViewManager::OnDestroyWebViewGuest)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestSetAutoSize, webviewManager, WebViewManager::OnWebViewGuestSetAutoSize)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestGo, webviewManager, WebViewManager::OnWebViewGuestGo)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestLoadUrl, webviewManager, WebViewManager::OnWebViewGuestLoadUrl)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestReload, webviewManager, WebViewManager::OnWebViewGuestReload)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestStop, webviewManager, WebViewManager::OnWebViewGuestStop)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestSetZoom, webviewManager, WebViewManager::OnWebViewGuestSetZoom)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestFind, webviewManager, WebViewManager::OnWebViewGuestFind)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestStopFinding, webviewManager, WebViewManager::OnWebViewGuestStopFinding)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestExecuteScript, webviewManager, WebViewManager::OnWebViewGuestExecuteScript)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestOpenDevTools, webviewManager, WebViewManager::OnWebViewGuestOpenDevTools)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestCloseDevTools, webviewManager, WebViewManager::OnWebViewGuestCloseDevTools)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestIsDevToolsOpened, webviewManager, WebViewManager::OnWebViewGuestIsDevToolsOpened)
      IPC_MESSAGE_FORWARD(MesonFrameHostMsg_WebViewGuestJavaScriptDialogClosed, webviewManager, WebViewManager::OnWebViewGuestJavaScriptDialogClosed)
    IPC_END_MESSAGE_MAP()
  }
#if 0
  IPC_BEGIN_MESSAGE_MAP(NativeWindow, message)
  IPC_END_MESSAGE_MAP()
#endif
  return handled;
}

void NativeWindow::OnCreateWebViewGuest(const base::DictionaryValue& params, int* guest_instance_id) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  auto wc = web_contents();
  WebViewManager::GetWebViewManager(wc)->OnCreateWebViewGuest(wc, params, guest_instance_id);
}
void NativeWindow::UpdateDraggableRegions(const std::vector<DraggableRegion>& regions) {
  // Draggable region is not supported for non-frameless window.
  if (has_frame_)
    return;
  draggable_region_ = DraggableRegionsToSkRegion(regions);
}

void NativeWindow::ScheduleUnresponsiveEvent(int ms) {
  if (!window_unresposive_closure_.IsCancelled())
    return;

  window_unresposive_closure_.Reset(base::Bind(&NativeWindow::NotifyWindowUnresponsive, weak_factory_.GetWeakPtr()));
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(FROM_HERE, window_unresposive_closure_.callback(), base::TimeDelta::FromMilliseconds(ms));
}

void NativeWindow::NotifyWindowUnresponsive() {
  window_unresposive_closure_.Cancel();

  if (!is_closed_ && !IsUnresponsiveEventSuppressed() && IsEnabled()) {
    for (NativeWindowObserver& observer : observers_)
      observer.OnRendererUnresponsive();
  }
}

void NativeWindow::NotifyReadyToShow() {
  for (NativeWindowObserver& observer : observers_)
    observer.OnReadyToShow();
}

}  // namespace atom
