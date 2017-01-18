//-*-c++-*-
#pragma once

#include <memory>
#include <map>
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/web_contents_observer.h"

#include "api/api_binding.h"
#include "browser/common_web_contents_delegate.h"
#include "content/public/common/favicon_url.h"
#include "content/common/cursors/webcursor.h"

namespace contents {
class WebContents;
}
namespace meson {
struct SetSizeParams;
class WebViewGuestDelegate;
class MesonWindow;
class MesonSessionBinding;
class MesonBrowserContext;
class MesonWebContentsBinding : public APIBindingT<MesonWebContentsBinding>,
                                public CommonWebContentsDelegate,
                                public content::WebContentsObserver {
 public:
  enum Type {
    BACKGROUND_PAGE,  // A DevTools extension background page.
    BROWSER_WINDOW,   // Used by BrowserWindow.
    REMOTE,           // Thin wrap around an existing WebContents.
    WEB_VIEW,         // Used by <webview>.
    OFF_SCREEN,       // Used for offscreen rendering
  };

 public:
  enum { ObjType = MESON_OBJECT_TYPE_WEB_CONTENTS };

 public:
  MesonWebContentsBinding(unsigned int id, const api::APICreateArg& args);
  virtual ~MesonWebContentsBinding(void);

 public:
  virtual void CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) override;

 public:
  int64_t GetWebContentsID() const;
  int GetProcessID() const;
  Type GetType() const;
  bool Equal(const MesonWebContentsBinding* web_contents) const;
  void LoadURL(const GURL& url, const base::DictionaryValue& options);
  void DownloadURL(const GURL& url);
  GURL GetURL() const;
  base::string16 GetTitle() const;
  bool IsLoading() const;
  bool IsLoadingMainFrame() const;
  bool IsWaitingForResponse() const;
  void Stop();
  void GoBack();
  void GoForward();
  void GoToOffset(int offset);
  bool IsCrashed() const;
  void SetUserAgent(const std::string& user_agent, base::ListValue* args);
  std::string GetUserAgent();
  void InsertCSS(const std::string& css);
#if 0
  //TODO:
  bool SavePage(const base::FilePath& full_file_path, const content::SavePageType& save_type, const SavePageHandler::SavePageCallback& callback);
#endif
  void OpenDevTools(base::ListValue* args);
  void CloseDevTools();
  bool IsDevToolsOpened();
  bool IsDevToolsFocused();
  void ToggleDevTools();
  void InspectElement(int x, int y);
  void InspectServiceWorker();
  void HasServiceWorker(const base::Callback<void(bool)>&);
  void UnregisterServiceWorker(const base::Callback<void(bool)>&);
  void SetAudioMuted(bool muted);
  bool IsAudioMuted();

  void AddWorkSpace(base::ListValue* args, const base::FilePath& path);
  void RemoveWorkSpace(base::ListValue* args, const base::FilePath& path);

  void Undo();
  void Redo();
  void Cut();
  void Copy();
  void Paste();
  void PasteAndMatchStyle();
  void Delete();
  void SelectAll();
  void Unselect();
  void Replace(const base::string16& word);
  void ReplaceMisspelling(const base::string16& word);
  uint32_t FindInPage(base::ListValue* args);
  void StopFindInPage(content::StopFindAction action);
  void ShowDefinitionForSelection();
  void CopyImageAt(int x, int y);
  // Focus.
  void Focus();
  bool IsFocused() const;
  void TabTraverse(bool reverse);
  // Send messages to browser.
  bool SendIPCMessage(bool all_frames, const base::string16& channel, const base::ListValue& args);
#if 0
  // Send WebInputEvent to the page.
  void SendInputEvent(v8::Isolate* isolate, v8::Local<v8::Value> input_event);
#endif
  // Subscribe to the frame updates.
  void BeginFrameSubscription(base::ListValue* args);
  void EndFrameSubscription();
  // Dragging native items.
  void StartDrag(const base::DictionaryValue& item, base::ListValue* args);
  // Captures the page with |rect|, |callback| would be called when capturing is done.
  void CapturePage(base::ListValue* args);
  bool IsGuest(void) const { return type_ == WEB_VIEW; }
  void SetSize(const SetSizeParams& params);
  bool IsOffscreen(void) const { return type_ == OFF_SCREEN; }
  void OnPaint(const gfx::Rect& dirty_rect, const SkBitmap& bitmap);
  void StartPainting();
  void StopPainting();
  bool IsPainting() const;
  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;
  void Invalidate();

  content::WebContents* HostWebContents();

  MesonWebContentsBinding* GetEmbedder(void) const {
    return embedder_.get();
  }
  void SetEmbedder(void);
  WebViewGuestDelegate* GetGuestDelegate() { return guest_delegate_.get(); };
public:
  void WebViewEmit(const std::string& type, const base::DictionaryValue& params);

 protected:
  // Create window with the given disposition.
  void OnCreateWindow(const GURL& target_url, const std::string& frame_name, WindowOpenDisposition disposition, const std::vector<base::string16>& features);
  // Callback triggered on permission response.
  void OnEnterFullscreenModeForTab(content::WebContents* source, const GURL& origin, bool allowed);

 protected:
  // content::WebContentsDelegate:
  bool AddMessageToConsole(content::WebContents* source, int32_t level, const base::string16& message, int32_t line_no, const base::string16& source_id) override;
  void WebContentsCreated(content::WebContents* source_contents, int opener_render_frame_id, const std::string& frame_name, const GURL& target_url, content::WebContents* new_contents) override;
  void AddNewContents(content::WebContents* source, content::WebContents* new_contents, WindowOpenDisposition disposition, const gfx::Rect& initial_rect, bool user_gesture, bool* was_blocked) override;
  content::WebContents* OpenURLFromTab(content::WebContents* source, const content::OpenURLParams& params) override;
  void BeforeUnloadFired(content::WebContents* tab, bool proceed, bool* proceed_to_fire_unload) override;
  void MoveContents(content::WebContents* source, const gfx::Rect& pos) override;
  void CloseContents(content::WebContents* source) override;
  void ActivateContents(content::WebContents* contents) override;
  void UpdateTargetURL(content::WebContents* source, const GURL& url) override;
  bool IsPopupOrPanel(const content::WebContents* source) const override;
  void HandleKeyboardEvent(content::WebContents* source, const content::NativeWebKeyboardEvent& event) override;
  void EnterFullscreenModeForTab(content::WebContents* source, const GURL& origin) override;
  void ExitFullscreenModeForTab(content::WebContents* source) override;
  void RendererUnresponsive(content::WebContents* source) override;
  void RendererResponsive(content::WebContents* source) override;
  bool HandleContextMenu(const content::ContextMenuParams& params) override;
  bool OnGoToEntryOffset(int offset) override;
  void FindReply(content::WebContents* web_contents, int request_id, int number_of_matches, const gfx::Rect& selection_rect, int active_match_ordinal, bool final_update) override;
  bool CheckMediaAccessPermission(content::WebContents* web_contents, const GURL& security_origin, content::MediaStreamType type) override;
  void RequestMediaAccessPermission(content::WebContents* web_contents, const content::MediaStreamRequest& request, const content::MediaResponseCallback& callback) override;
  void RequestToLockMouse(content::WebContents* web_contents, bool user_gesture, bool last_unlocked_by_target) override;
  std::unique_ptr<content::BluetoothChooser> RunBluetoothChooser(content::RenderFrameHost* frame, const content::BluetoothChooser::EventHandler& handler) override;

  // content::WebContentsObserver:
  void BeforeUnloadFired(const base::TimeTicks& proceed_time) override;
  void RenderViewDeleted(content::RenderViewHost*) override;
  void RenderProcessGone(base::TerminationStatus status) override;
  void DocumentLoadedInFrame(content::RenderFrameHost* render_frame_host) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host, const GURL& validated_url) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host, const GURL& validated_url, int error_code, const base::string16& error_description, bool was_ignored_by_handler) override;
  void DidStartLoading() override;
  void DidStopLoading() override;
  void DidGetResourceResponseStart(const content::ResourceRequestDetails& details) override;
  void DidGetRedirectForResourceRequest(content::RenderFrameHost* render_frame_host, const content::ResourceRedirectDetails& details) override;
  void DidFinishNavigation(content::NavigationHandle* navigation_handle) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void WebContentsDestroyed() override;
  void NavigationEntryCommitted(const content::LoadCommittedDetails& load_details) override;
  void TitleWasSet(content::NavigationEntry* entry, bool explicit_set) override;
  void DidUpdateFaviconURL(const std::vector<content::FaviconURL>& urls) override;
  void PluginCrashed(const base::FilePath& plugin_path, base::ProcessId plugin_pid) override;
  void MediaStartedPlaying(const MediaPlayerId& id) override;
  void MediaStoppedPlaying(const MediaPlayerId& id) override;
  void DidChangeThemeColor(SkColor theme_color) override;

  // brightray::InspectableWebContentsDelegate:
  void DevToolsReloadPage() override;

  // brightray::InspectableWebContentsViewDelegate:
  void DevToolsFocused() override;
  void DevToolsOpened() override;
  void DevToolsClosed() override;

 private:
  MesonBrowserContext* GetBrowserContext() const;
  // Called when we receive a CursorChange message from chromium.

  void OnCursorChange(const content::WebCursor& cursor);
  // Called when received a message from renderer.
  void OnRendererMessage(const base::string16& channel, const base::ListValue& args);
  // Called when received a synchronous message from renderer.
  void OnRendererMessageSync(const base::string16& channel, const base::ListValue& args, IPC::Message* message);
  void WebContentsDestroyedCore(bool destuctor);

 private:
  //scoped_refptr<MesonWebContentsBinding> embedder_;
  base::WeakPtr<MesonWebContentsBinding> embedder_;
  Type type_;
  //unsigned int request_id_;
  bool background_throttling_;
  bool enable_devtools_;
  scoped_refptr<MesonSessionBinding> session_;
  std::unique_ptr<WebViewGuestDelegate> guest_delegate_;
  int guest_instance_id_;
};

class MesonWebContentsBindingFactory : public APIBindingFactory {
 public:
  MesonWebContentsBindingFactory(void);
  virtual ~MesonWebContentsBindingFactory(void);

 public:
  virtual APIBinding* Create(unsigned int id, const api::APICreateArg& args) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MesonWebContentsBindingFactory);
};
}
