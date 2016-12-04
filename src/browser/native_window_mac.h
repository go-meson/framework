//-*-objc-mode-*-
#pragma once

#import <Cocoa/Cocoa.h>

#include <string>
#include <vector>

#include "browser/native_window.h"
#include "base/mac/scoped_nsobject.h"
#include "content/public/browser/render_widget_host.h"

@class MesonNSWindow;
@class MesonNSWindowDelegate;
@class FullSizeContentView;

namespace meson {
class NativeWindowMac : public NativeWindow,
                        public content::RenderWidgetHost::InputEventObserver {
 public:
  NativeWindowMac(brightray::InspectableWebContents* inspectable_web_contents, const base::DictionaryValue& options, NativeWindow* parent);
  ~NativeWindowMac() override;

 public:
  void Close() override;
  void CloseImmediately() override;
  void Focus(bool focus) override;
  bool IsFocused() override;
  void Show() override;
  void ShowInactive() override;
  void Hide() override;
  bool IsVisible() override;
  bool IsEnabled() override;
  void Maximize() override;
  void Unmaximize() override;
  bool IsMaximized() override;
  void Minimize() override;
  void Restore() override;
  bool IsMinimized() override;
  void SetFullScreen(bool fullscreen) override;
  bool IsFullscreen() const override;
  void SetBounds(const gfx::Rect& bounds, bool animate = false) override;
  gfx::Rect GetBounds() override;
  void SetContentSizeConstraints(const extensions::SizeConstraints& size_constraints) override;
  void SetResizable(bool resizable) override;
  bool IsResizable() override;
  void SetMovable(bool movable) override;
  void SetAspectRatio(double aspect_ratio, const gfx::Size& extra_size) override;
  bool IsMovable() override;
  void SetMinimizable(bool minimizable) override;
  bool IsMinimizable() override;
  void SetMaximizable(bool maximizable) override;
  bool IsMaximizable() override;
  void SetFullScreenable(bool fullscreenable) override;
  bool IsFullScreenable() override;
  void SetClosable(bool closable) override;
  bool IsClosable() override;
  void SetAlwaysOnTop(bool top, const std::string& level) override;
  bool IsAlwaysOnTop() override;
  void Center() override;
  void SetTitle(const std::string& title) override;
  std::string GetTitle() override;
  void FlashFrame(bool flash) override;
  void SetSkipTaskbar(bool skip) override;
  void SetKiosk(bool kiosk) override;
  bool IsKiosk() override;
  void SetBackgroundColor(const std::string& color_name) override;
  void SetHasShadow(bool has_shadow) override;
  bool HasShadow() override;
  void SetRepresentedFilename(const std::string& filename) override;
  std::string GetRepresentedFilename() override;
  void SetDocumentEdited(bool edited) override;
  bool IsDocumentEdited() override;
  void SetIgnoreMouseEvents(bool ignore) override;
  void SetContentProtection(bool enable) override;
  void SetParentWindow(NativeWindow* parent) override;
  gfx::NativeWindow GetNativeWindow() override;
  gfx::AcceleratedWidget GetAcceleratedWidget() override;
  void SetProgressBar(double progress, const ProgressState state) override;
  void SetOverlayIcon(const gfx::Image& overlay, const std::string& description) override;
  void SetVisibleOnAllWorkspaces(bool visible) override;
  bool IsVisibleOnAllWorkspaces() override;

 public:
  void OnInputEvent(const blink::WebInputEvent& event) override;

 public:
  void RenderViewHostChanged(content::RenderViewHost* old_host, content::RenderViewHost* new_host) override;
  void UpdateDraggableRegionViews() { UpdateDraggableRegionViews(draggable_regions_); }

 public:
  void SetStyleMask(bool on, NSUInteger flag);
  void SetCollectionBehavior(bool on, NSUInteger flag);

 public:
  enum TitleBarStyle {
    NORMAL,
    HIDDEN,
    HIDDEN_INSET,
  };
  TitleBarStyle title_bar_style() const { return title_bar_style_; }

 protected:
  // Return a vector of non-draggable regions that fill a window of size
  // |width| by |height|, but leave gaps where the window should be draggable.
  std::vector<gfx::Rect> CalculateNonDraggableRegions(const std::vector<DraggableRegion>& regions, int width, int height);

 private:
  gfx::Rect ContentBoundsToWindowBounds(const gfx::Rect& bounds);
  gfx::Rect WindowBoundsToContentBounds(const gfx::Rect& bounds);
  void UpdateDraggableRegions(const std::vector<DraggableRegion>& regions) override;
  void ShowWindowButton(NSWindowButton button);
  void InstallView();
  void UninstallView();
  void UpdateDraggableRegionViews(const std::vector<DraggableRegion>& regions);
  void RegisterInputEventObserver(content::RenderViewHost* host);
  void UnregisterInputEventObserver(content::RenderViewHost* host);

 private:
  base::scoped_nsobject<MesonNSWindow> window_;
  base::scoped_nsobject<MesonNSWindowDelegate> window_delegate_;
  id wheel_event_monitor_;
  base::scoped_nsobject<FullSizeContentView> content_view_;
  std::vector<DraggableRegion> draggable_regions_;
  bool is_kiosk_;
  NSInteger attention_request_id_;
  NSApplicationPresentationOptions kiosk_options_;
  TitleBarStyle title_bar_style_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowMac);
};
}
