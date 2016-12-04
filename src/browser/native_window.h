//-*-c++-*-
#pragma once

#include <memory>
#include <vector>

#include "browser/native_window_observer.h"
//#include "browser/ui/accelerator_util.h"
//#include "browser/ui/meson_menu_model.h"
#include "base/cancelable_callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/supports_user_data.h"
#include "content/public/browser/readback_types.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/app_window/size_constraints.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"

namespace brightray {
class InspectableWebContents;
}
namespace content {
struct NativeWebKeyboardEvent;
}

namespace meson {
struct DraggableRegion;
class MesonMenuModel;
class NativeWindow : public base::SupportsUserData, public content::WebContentsObserver {
 public:
  ~NativeWindow() override;

  static NativeWindow* Create(brightray::InspectableWebContents* inspectable_web_contents,
                              const base::DictionaryValue& options,
                              NativeWindow* parent = nullptr);
  static NativeWindow* FromWebContents(content::WebContents* web_contents);

  void InitFromOptions(const base::DictionaryValue& options);

 public:
  virtual void Close() = 0;
  virtual void CloseImmediately() = 0;
  virtual bool IsClosed() const { return is_closed_; }
  virtual void Focus(bool focus) = 0;
  virtual bool IsFocused() = 0;
  virtual void Show() = 0;
  virtual void ShowInactive() = 0;
  virtual void Hide() = 0;
  virtual bool IsVisible() = 0;
  virtual bool IsEnabled() = 0;
  virtual void Maximize() = 0;
  virtual void Unmaximize() = 0;
  virtual bool IsMaximized() = 0;
  virtual void Minimize() = 0;
  virtual void Restore() = 0;
  virtual bool IsMinimized() = 0;
  virtual void SetFullScreen(bool fullscreen) = 0;
  virtual bool IsFullscreen() const = 0;
  virtual void SetBounds(const gfx::Rect& bounds, bool animate = false) = 0;
  virtual gfx::Rect GetBounds() = 0;
  virtual void SetSize(const gfx::Size& size, bool animate = false);
  virtual gfx::Size GetSize();
  virtual void SetPosition(const gfx::Point& position, bool animate = false);
  virtual gfx::Point GetPosition();
  virtual void SetContentSize(const gfx::Size& size, bool animate = false);
  virtual gfx::Size GetContentSize();
  virtual void SetContentBounds(const gfx::Rect& bounds, bool animate = false);
  virtual gfx::Rect GetContentBounds();
  virtual void SetSizeConstraints(const extensions::SizeConstraints& size_constraints);
  virtual extensions::SizeConstraints GetSizeConstraints();
  virtual void SetContentSizeConstraints(const extensions::SizeConstraints& size_constraints);
  virtual extensions::SizeConstraints GetContentSizeConstraints();
  virtual void SetMinimumSize(const gfx::Size& size);
  virtual gfx::Size GetMinimumSize();
  virtual void SetMaximumSize(const gfx::Size& size);
  virtual gfx::Size GetMaximumSize();
  virtual void SetSheetOffset(const double offsetX, const double offsetY);
  virtual double GetSheetOffsetX();
  virtual double GetSheetOffsetY();
  virtual void SetResizable(bool resizable) = 0;
  virtual bool IsResizable() = 0;
  virtual void SetMovable(bool movable) = 0;
  virtual bool IsMovable() = 0;
  virtual void SetMinimizable(bool minimizable) = 0;
  virtual bool IsMinimizable() = 0;
  virtual void SetMaximizable(bool maximizable) = 0;
  virtual bool IsMaximizable() = 0;
  virtual void SetFullScreenable(bool fullscreenable) = 0;
  virtual bool IsFullScreenable() = 0;
  virtual void SetClosable(bool closable) = 0;
  virtual bool IsClosable() = 0;
  virtual void SetAlwaysOnTop(bool top, const std::string& level = "floating") = 0;
  virtual bool IsAlwaysOnTop() = 0;
  virtual void Center() = 0;
  virtual void SetTitle(const std::string& title) = 0;
  virtual std::string GetTitle() = 0;
  virtual void FlashFrame(bool flash) = 0;
  virtual void SetSkipTaskbar(bool skip) = 0;
  virtual void SetKiosk(bool kiosk) = 0;
  virtual bool IsKiosk() = 0;
  virtual void SetBackgroundColor(const std::string& color_name) = 0;
  virtual void SetHasShadow(bool has_shadow) = 0;
  virtual bool HasShadow() = 0;
  virtual void SetRepresentedFilename(const std::string& filename);
  virtual std::string GetRepresentedFilename();
  virtual void SetDocumentEdited(bool edited);
  virtual bool IsDocumentEdited();
  virtual void SetIgnoreMouseEvents(bool ignore) = 0;
  virtual void SetContentProtection(bool enable) = 0;
  virtual void SetFocusable(bool focusable);
  virtual void SetMenu(MesonMenuModel* menu);
  virtual void SetParentWindow(NativeWindow* parent);
  virtual gfx::NativeWindow GetNativeWindow() = 0;
  virtual gfx::AcceleratedWidget GetAcceleratedWidget() = 0;

 public:
  enum ProgressState {
    PROGRESS_NONE,           // no progress, no marking
    PROGRESS_INDETERMINATE,  // progress, indeterminate
    PROGRESS_ERROR,          // progress, errored (red)
    PROGRESS_PAUSED,         // progress, paused (yellow)
    PROGRESS_NORMAL,         // progress, not marked (green)
  };
  virtual void SetProgressBar(double progress, const ProgressState state) = 0;
  virtual void SetOverlayIcon(const gfx::Image& overlay, const std::string& description) = 0;

 public:
  virtual void SetVisibleOnAllWorkspaces(bool visible) = 0;
  virtual bool IsVisibleOnAllWorkspaces() = 0;

 public:
  virtual void FocusOnWebView();
  virtual void BlurWebView();
  virtual bool IsWebViewFocused();

 public:
  virtual void SetAutoHideMenuBar(bool auto_hide);
  virtual bool IsMenuBarAutoHide();
  virtual void SetMenuBarVisibility(bool visible);
  virtual bool IsMenuBarVisible();

 public:
  double GetAspectRatio();
  gfx::Size GetAspectRatioExtraSize();
  virtual void SetAspectRatio(double aspect_ratio, const gfx::Size& extra_size);

 public:
  base::WeakPtr<NativeWindow> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

 public:
  virtual void RequestToClosePage();

 public:
  virtual void CloseContents(content::WebContents* source);
  virtual void RendererUnresponsive(content::WebContents* source);
  virtual void RendererResponsive(content::WebContents* source);
  virtual void HandleKeyboardEvent(content::WebContents*, const content::NativeWebKeyboardEvent& event) {}

 public:
  void NotifyWindowClosed();
  void NotifyWindowBlur();
  void NotifyWindowFocus();
  void NotifyWindowShow();
  void NotifyWindowHide();
  void NotifyWindowMaximize();
  void NotifyWindowUnmaximize();
  void NotifyWindowMinimize();
  void NotifyWindowRestore();
  void NotifyWindowMove();
  void NotifyWindowResize();
  void NotifyWindowMoved();
  void NotifyWindowScrollTouchBegin();
  void NotifyWindowScrollTouchEnd();
  void NotifyWindowScrollTouchEdge();
  void NotifyWindowSwipe(const std::string& direction);
  void NotifyWindowEnterFullScreen();
  void NotifyWindowLeaveFullScreen();
  void NotifyWindowEnterHtmlFullScreen();
  void NotifyWindowLeaveHtmlFullScreen();
  void NotifyWindowExecuteWindowsCommand(const std::string& command);
#if defined(OS_WIN)
  void NotifyWindowMessage(UINT message, WPARAM w_param, LPARAM l_param);
#endif

 public:
  void AddObserver(NativeWindowObserver* obs) { observers_.AddObserver(obs); }
  void RemoveObserver(NativeWindowObserver* obs) { observers_.RemoveObserver(obs); }

 public:
  brightray::InspectableWebContents* inspectable_web_contents() const { return inspectable_web_contents_; }

 public:
  bool has_frame() const { return has_frame_; }
  void set_has_frame(bool has_frame) { has_frame_ = has_frame; }

  bool transparent() const { return transparent_; }
  SkRegion* draggable_region() const { return draggable_region_.get(); }
  bool enable_larger_than_screen() const { return enable_larger_than_screen_; }

  NativeWindow* parent() const { return parent_; }
  bool is_modal() const { return is_modal_; }

 protected:
  NativeWindow(brightray::InspectableWebContents* inspectable_web_contents, const base::DictionaryValue& options, NativeWindow* parent);
  std::unique_ptr<SkRegion> DraggableRegionsToSkRegion(const std::vector<DraggableRegion>& regions);
  virtual gfx::Rect ContentBoundsToWindowBounds(const gfx::Rect& bounds) = 0;
  virtual gfx::Rect WindowBoundsToContentBounds(const gfx::Rect& bounds) = 0;
  virtual void UpdateDraggableRegions(const std::vector<DraggableRegion>& regions);

 protected:
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void BeforeUnloadDialogCancelled() override;
  void DidFirstVisuallyNonEmptyPaint() override;
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  void ScheduleUnresponsiveEvent(int ms);
  void NotifyWindowUnresponsive();
  void NotifyReadyToShow();
  bool has_frame_;
  bool transparent_;
  std::unique_ptr<SkRegion> draggable_region_;  // used in custom drag.
  extensions::SizeConstraints size_constraints_;
  bool enable_larger_than_screen_;
  bool is_closed_;
  base::CancelableClosure window_unresposive_closure_;
  double sheet_offset_x_;
  double sheet_offset_y_;
  double aspect_ratio_;
  gfx::Size aspect_ratio_extraSize_;
  NativeWindow* parent_;
  bool is_modal_;
  brightray::InspectableWebContents* inspectable_web_contents_;
  base::ObserverList<NativeWindowObserver> observers_;
  base::WeakPtrFactory<NativeWindow> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindow);
};

// This class provides a hook to get a NativeWindow from a WebContents.
class NativeWindowRelay : public content::WebContentsUserData<NativeWindowRelay> {
 public:
  explicit NativeWindowRelay(base::WeakPtr<NativeWindow> window)
      : key(UserDataKey()), window(window) {}

  void* key;
  base::WeakPtr<NativeWindow> window;

 private:
  friend class content::WebContentsUserData<NativeWindow>;
};
}
