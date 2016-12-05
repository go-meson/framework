#include "api/web_contents_binding.h"

#include "brightray/browser/inspectable_web_contents_view.h"

#include "api/session_binding.h"
#include "api/api.h"
#include "api/api_messages.h"
#include "browser/web_contents_permission_helper.h"
#include "browser/web_contents_preferences.h"
#include "browser/meson_security_state_model_client.h"
#include "browser/native_window.h"
#include "browser/browser_client.h"
#include "common/options_switches.h"
#include "common/color_util.h"
#include "content/public/common/context_menu_params.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/resource_request_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/download_manager.h"
#include "content/common/view_messages.h"
#include "base/strings/utf_string_conversions.h"

namespace meson {
template <>
APIBindingT<MesonWebContentsBinding>::MethodTable APIBindingT<MesonWebContentsBinding>::methodTable = {};

MesonWebContentsBinding::MesonWebContentsBinding(unsigned int id, const api::APICreateArg& args)
    : APIBindingT(MESON_OBJECT_TYPE_WEB_CONTENTS, id),
      embedder_(nullptr),
      type_(BROWSER_WINDOW),
      /*request_id_(0),*/ background_throttling_(true),
      enable_devtools_(true) {
  args.GetBoolean("backgroundThrottling", &background_throttling_);
  bool b = false;
  if (args.GetBoolean("isGuest", &b) && b) {
    type_ = WEB_VIEW;
  } else if (args.GetBoolean("isBackgroundPage", &b) && b) {
    type_ = BACKGROUND_PAGE;
  } else if (args.GetBoolean("option", &b) && b) {
    type_ = OFF_SCREEN;
  }
  args.GetBoolean("devTools", &enable_devtools_);

  int session_id;
  if (args.GetInteger("_session_id_", &session_id)) {
    session_ = static_cast<MesonSessionBinding*>(API::Get()->GetBinding(session_id).get());
  } else {
    base::DictionaryValue sessionArg;
    std::string partition;
    if (args.GetString("partition", &partition)) {
      sessionArg.SetString("partition", partition);
    }
    session_ = API::Get()->Create<MesonSessionBinding>(MESON_OBJECT_TYPE_SESSION, sessionArg);
  }
  content::WebContents* web_contents;
#if 0
  if (IsGuest()) {
    auto ctx = session_->GetSession();
    auto site_instance = content::SiteInstance::CreateForURL(ctx.get(), GURL("chrome-guest::/fake-host"));
    content::WebContents::CreateParams params(ctx.get(), site_instance);
    assert(false);
    guest_delegate_.reset(new WebViewGuestDelegate);
    params.guest_delegate = guest_delegate_.get();
    web_contents = content::WebContents::Create(params);
  } else if (IsOffscreen()) {
    bool transparent = false;
    args->GetBoolean("transparent", &transparent);
    content::WebContents::CreateParams params(session_->GetSession().get());
    assert(false);
    auto* view = new OffScreenWebContentsView(transparent, base::Bind(&WebContents::OnPaint, base::Unretained(this)));
    params.view = view;
    params.delegate_view = view;

    web_contents = content::WebContents::Create(params);
    view->SetWebContents(web_contents);
  } else {
#endif
  content::WebContents::CreateParams params(session_->GetSession().get());
  web_contents = content::WebContents::Create(params);
#if 0
  }
#endif
  Observe(web_contents);

  //InitWithSessionAndOptions(isolate, web_contents, session, options);
  InitWithWebContents(web_contents, session_->GetSession().get());

  managed_web_contents()->GetView()->SetDelegate(this);

  // Save the preferences in C++.
  new WebContentsPreferences(web_contents, args);

  WebContentsPermissionHelper::CreateForWebContents(web_contents);
  MesonSecurityStateModelClient::CreateForWebContents(web_contents);

  web_contents->SetUserAgentOverride(GetBrowserContext()->GetUserAgent());

#if 0
  if (IsGuest()) {
    guest_delegate_->Initialize(this);

    NativeWindow* owner_window = nullptr;
    if (options.Get("embedder", &embedder_) && embedder_) {
      // New WebContents's owner_window is the embedder's owner_window.
      auto relay =
        NativeWindowRelay::FromWebContents(embedder_->web_contents());
      if (relay)
        owner_window = relay->window.get();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);
  }
#endif
  //AttachAsUserData(web_contents);
}

MesonWebContentsBinding::~MesonWebContentsBinding(void) {}
void MesonWebContentsBinding::CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) {
}

MesonBrowserContext* MesonWebContentsBinding::GetBrowserContext() const {
  return static_cast<MesonBrowserContext*>(web_contents()->GetBrowserContext());
}

#if 0
template <typename F, typename... R>
bool Emit(F f, R...) {
  LOG(INFO) << "Emit:" << f;
  return false;
}
template <typename F, typename... R>
bool EmitWithSender(F f, R...) {
  LOG(INFO) << "EmitWithSender:" << f;
  return false;
}
#endif

bool MesonWebContentsBinding::AddMessageToConsole(content::WebContents* source,
                                                  int32_t level,
                                                  const base::string16& message,
                                                  int32_t line_no,
                                                  const base::string16& source_id) {
  if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN) {
    return false;
  } else {
    EmitEvent("console-message", level, message, line_no, source_id);
    return true;
  }
}

void MesonWebContentsBinding::OnCreateWindow(const GURL& target_url,
                                             const std::string& frame_name,
                                             WindowOpenDisposition disposition,
                                             const std::vector<base::string16>& features) {
  if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN)
    EmitEvent("-new-window", target_url, frame_name, disposition, features);
  else
    EmitEvent("new-window", target_url, frame_name, disposition, features);
}

void MesonWebContentsBinding::WebContentsCreated(content::WebContents* source_contents,
                                                 int opener_render_frame_id,
                                                 const std::string& frame_name,
                                                 const GURL& target_url,
                                                 content::WebContents* new_contents) {
#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto api_web_contents = CreateFrom(isolate(), new_contents, BROWSER_WINDOW);
#else
  auto api_web_contents = GetID();
#endif
  EmitEvent("-web-contents-created", api_web_contents, target_url, frame_name);
}

void MesonWebContentsBinding::AddNewContents(content::WebContents* source,
                                             content::WebContents* new_contents,
                                             WindowOpenDisposition disposition,
                                             const gfx::Rect& initial_rect,
                                             bool user_gesture,
                                             bool* was_blocked) {
#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto api_web_contents = CreateFrom(isolate(), new_contents);
#else
  auto api_web_contents = GetID();
#endif
  EmitEvent("-add-new-contents", api_web_contents, disposition, user_gesture,
       initial_rect.x(), initial_rect.y(), initial_rect.width(),
       initial_rect.height());
}

content::WebContents* MesonWebContentsBinding::OpenURLFromTab(content::WebContents* source,
                                                              const content::OpenURLParams& params) {
  if (params.disposition != CURRENT_TAB) {
    if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN)
      EmitEvent("-new-window", params.url, "", params.disposition);
    else
      EmitEvent("new-window", params.url, "", params.disposition);
    return nullptr;
  }

  // Give user a chance to cancel navigation.
  bool prevent = false;
  //TODO:
  EmitEvent("will-navigate", params.url);
  if (prevent)
    return nullptr;

  return CommonWebContentsDelegate::OpenURLFromTab(source, params);
}

void MesonWebContentsBinding::BeforeUnloadFired(content::WebContents* tab,
                                                bool proceed,
                                                bool* proceed_to_fire_unload) {
  if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN)
    *proceed_to_fire_unload = proceed;
  else
    *proceed_to_fire_unload = true;
}

void MesonWebContentsBinding::MoveContents(content::WebContents* source,
                                           const gfx::Rect& pos) {
  EmitEvent("move", pos);
}

void MesonWebContentsBinding::CloseContents(content::WebContents* source) {
  EmitEvent("close");

  if ((type_ == BROWSER_WINDOW || type_ == OFF_SCREEN) && owner_window()) {
    owner_window()->CloseContents(source);
  }
}

void MesonWebContentsBinding::ActivateContents(content::WebContents* source) {
  EmitEvent("activate");
}

void MesonWebContentsBinding::UpdateTargetURL(content::WebContents* source,
                                              const GURL& url) {
  EmitEvent("update-target-url", url);
}

bool MesonWebContentsBinding::IsPopupOrPanel(const content::WebContents* source) const {
  return type_ == BROWSER_WINDOW;
}

void MesonWebContentsBinding::HandleKeyboardEvent(content::WebContents* source, const content::NativeWebKeyboardEvent& event) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (type_ == WEB_VIEW && embedder_) {
    // Send the unhandled keyboard events back to the embedder.
    embedder_->HandleKeyboardEvent(source, event);
  } else {
    // Go to the default keyboard handling.
    CommonWebContentsDelegate::HandleKeyboardEvent(source, event);
  }
}

void MesonWebContentsBinding::EnterFullscreenModeForTab(content::WebContents* source, const GURL& origin) {
  auto permission_helper = WebContentsPermissionHelper::FromWebContents(source);
  auto callback = base::Bind(&MesonWebContentsBinding::OnEnterFullscreenModeForTab, base::Unretained(this), source, origin);
  permission_helper->RequestFullscreenPermission(callback);
}

void MesonWebContentsBinding::OnEnterFullscreenModeForTab(content::WebContents* source, const GURL& origin, bool allowed) {
  if (!allowed)
    return;
  CommonWebContentsDelegate::EnterFullscreenModeForTab(source, origin);
  EmitEvent("enter-html-full-screen");
}

void MesonWebContentsBinding::ExitFullscreenModeForTab(content::WebContents* source) {
  CommonWebContentsDelegate::ExitFullscreenModeForTab(source);
  EmitEvent("leave-html-full-screen");
}

void MesonWebContentsBinding::RendererUnresponsive(content::WebContents* source) {
  EmitEvent("unresponsive");
  if ((type_ == BROWSER_WINDOW || type_ == OFF_SCREEN) && owner_window())
    owner_window()->RendererUnresponsive(source);
}

void MesonWebContentsBinding::RendererResponsive(content::WebContents* source) {
  EmitEvent("responsive");
  if ((type_ == BROWSER_WINDOW || type_ == OFF_SCREEN) && owner_window())
    owner_window()->RendererResponsive(source);
}

bool MesonWebContentsBinding::HandleContextMenu(const content::ContextMenuParams& params) {
  if (params.custom_context.is_pepper_menu) {
    //TODO:
    //EmitEvent("pepper-context-menu", std::make_pair(params, web_contents()));
    EmitEvent("pepper-context-menu");
    web_contents()->NotifyContextMenuClosed(params.custom_context);
  } else {
    //TODO:
    //EmitEvent("context-menu", std::make_pair(params, web_contents()));
    EmitEvent("context-menu");
  }

  return true;
}

bool MesonWebContentsBinding::OnGoToEntryOffset(int offset) {
  GoToOffset(offset);
  return false;
}

void MesonWebContentsBinding::FindReply(content::WebContents* web_contents,
                                        int request_id,
                                        int number_of_matches,
                                        const gfx::Rect& selection_rect,
                                        int active_match_ordinal,
                                        bool final_update) {
  if (!final_update)
    return;

#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  mate::Dictionary result = mate::Dictionary::CreateEmpty(isolate());
#else
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
#endif
  result->SetInteger("requestId", request_id);
  result->SetInteger("matches", number_of_matches);
  std::unique_ptr<base::DictionaryValue> rect(new base::DictionaryValue());
  rect->SetInteger("x", selection_rect.x());
  rect->SetInteger("y", selection_rect.y());
  rect->SetInteger("width", selection_rect.width());
  rect->SetInteger("height", selection_rect.height());
  result->Set("selectionArea", std::move(rect));
  result->SetInteger("activeMatchOrdinal", active_match_ordinal);
  result->SetBoolean("finalUpdate", final_update);  // Deprecate after 2.0
  EmitEvent("found-in-page", result.get());
}

bool MesonWebContentsBinding::CheckMediaAccessPermission(content::WebContents* web_contents, const GURL& security_origin, content::MediaStreamType type) {
  return true;
}

void MesonWebContentsBinding::RequestMediaAccessPermission(content::WebContents* web_contents,
                                                           const content::MediaStreamRequest& request,
                                                           const content::MediaResponseCallback& callback) {
  auto permission_helper = WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestMediaAccessPermission(request, callback);
}

void MesonWebContentsBinding::RequestToLockMouse(content::WebContents* web_contents, bool user_gesture, bool last_unlocked_by_target) {
  auto permission_helper = WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestPointerLockPermission(user_gesture);
}

std::unique_ptr<content::BluetoothChooser> MesonWebContentsBinding::RunBluetoothChooser(content::RenderFrameHost* frame, const content::BluetoothChooser::EventHandler& event_handler) {
#if 0
  //TODO:
  std::unique_ptr<BluetoothChooser> bluetooth_chooser(new BluetoothChooser(this, event_handler));
  return std::move(bluetooth_chooser);
#else
  return nullptr;
#endif
}

void MesonWebContentsBinding::BeforeUnloadFired(const base::TimeTicks& proceed_time) {
  // Do nothing, we override this method just to avoid compilation error since
  // there are two virtual functions named BeforeUnloadFired.
}

void MesonWebContentsBinding::RenderViewDeleted(content::RenderViewHost* render_view_host) {
  EmitEvent("render-view-deleted", render_view_host->GetProcess()->GetID());
}

void MesonWebContentsBinding::RenderProcessGone(base::TerminationStatus status) {
  EmitEvent("crashed", status == base::TERMINATION_STATUS_PROCESS_WAS_KILLED);
}

void MesonWebContentsBinding::PluginCrashed(const base::FilePath& plugin_path, base::ProcessId plugin_pid) {
//TODO:
#if 0
  content::WebPluginInfo info;
  auto plugin_service = content::PluginService::GetInstance();
  plugin_service->GetPluginInfoByPath(plugin_path, &info);
  EmitEvent("plugin-crashed", info.name, info.version);
#else
  EmitEvent("plugin-crashed");
#endif
}

void MesonWebContentsBinding::MediaStartedPlaying(const MediaPlayerId& id) {
  EmitEvent("media-started-playing");
}

void MesonWebContentsBinding::MediaStoppedPlaying(const MediaPlayerId& id) {
  EmitEvent("media-paused");
}

void MesonWebContentsBinding::DidChangeThemeColor(SkColor theme_color) {
  EmitEvent("did-change-theme-color", meson::ToRGBHex(theme_color));
}

void MesonWebContentsBinding::DocumentLoadedInFrame(content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host->GetParent())
    EmitEvent("dom-ready");
}

void MesonWebContentsBinding::DidFinishLoad(content::RenderFrameHost* render_frame_host, const GURL& validated_url) {
  bool is_main_frame = !render_frame_host->GetParent();
  EmitEvent("did-frame-finish-load", is_main_frame);

  if (is_main_frame)
    EmitEvent("did-finish-load");
}

void MesonWebContentsBinding::DidFailLoad(content::RenderFrameHost* render_frame_host,
                                          const GURL& url,
                                          int error_code,
                                          const base::string16& error_description,
                                          bool was_ignored_by_handler) {
  bool is_main_frame = !render_frame_host->GetParent();
  EmitEvent("did-fail-load", error_code, error_description, url, is_main_frame);
}

void MesonWebContentsBinding::DidStartLoading() {
  EmitEvent("did-start-loading");
}

void MesonWebContentsBinding::DidStopLoading() {
  EmitEvent("did-stop-loading");
}

void MesonWebContentsBinding::DidGetResourceResponseStart(const content::ResourceRequestDetails& details) {
#if 0
  //TODO:これ、ログの表示がウザいな……
  //常にイベントを戻すと面倒か?
  Emit("did-get-response-details", details.socket_address.IsEmpty(), details.url, details.original_url, details.http_response_code,
       details.method, details.referrer, details.headers.get() /*,  TODO: ResourceTypeToString(details.resource_type)*/);
#endif
}

void MesonWebContentsBinding::DidGetRedirectForResourceRequest(content::RenderFrameHost* render_frame_host, const content::ResourceRedirectDetails& details) {
  EmitEvent("did-get-redirect-request", details.url, details.new_url, (details.resource_type == content::RESOURCE_TYPE_MAIN_FRAME),
       details.http_response_code, details.method, details.referrer, details.headers.get());
}

void MesonWebContentsBinding::DidFinishNavigation(content::NavigationHandle* navigation_handle) {
  bool is_main_frame = navigation_handle->IsInMainFrame();
  if (navigation_handle->HasCommitted() && !navigation_handle->IsErrorPage()) {
    auto url = navigation_handle->GetURL();
    bool is_in_page = navigation_handle->IsSamePage();
    if (is_main_frame && !is_in_page) {
      EmitEvent("did-navigate", url);
    } else if (is_in_page) {
      EmitEvent("did-navigate-in-page", url, is_main_frame);
    }
  } else {
    auto url = navigation_handle->GetURL();
    int code = navigation_handle->GetNetErrorCode();
    auto description = net::ErrorToShortString(code);
    EmitEvent("did-fail-provisional-load", code, description, url, is_main_frame);

    // Do not emit "did-fail-load" for canceled requests.
    if (code != net::ERR_ABORTED)
      EmitEvent("did-fail-load", code, description, url, is_main_frame);
  }
}

void MesonWebContentsBinding::TitleWasSet(content::NavigationEntry* entry, bool explicit_set) {
  if (entry)
    EmitEvent("-page-title-updated", entry->GetTitle(), explicit_set);
  else
    EmitEvent("-page-title-updated", "", explicit_set);
}

void MesonWebContentsBinding::DidUpdateFaviconURL(const std::vector<content::FaviconURL>& urls) {
#if 0
  //TODO:
  std::set<GURL> unique_urls;
  for (const auto& iter : urls) {
    if (iter.icon_type != content::FaviconURL::FAVICON)
      continue;
    const GURL& url = iter.icon_url;
    if (url.is_valid())
      unique_urls.insert(url);
  }
  EmitEvent("page-favicon-updated", unique_urls);
#else
  EmitEvent("page-favicon-updated");
#endif
}

void MesonWebContentsBinding::DevToolsReloadPage() {
  EmitEvent("devtools-reload-page");
}

void MesonWebContentsBinding::DevToolsFocused() {
  EmitEvent("devtools-focused");
}

void MesonWebContentsBinding::DevToolsOpened() {
//TODO:
#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto handle = MesonWebContentsBinding::CreateFrom(isolate(), managed_web_contents()->GetDevToolsWebContents());
  auto c = managed_web_contents()->GetDevToolsWebContents();
  auto handle = API::Get()->Create(MesonWebContentsBinding::Name, c);
  devtools_web_contents_.Reset(isolate(), handle.ToV8());

  // Set inspected tabID.
  base::FundamentalValue tab_id(ID());
  managed_web_contents()->CallClientFunction("DevToolsAPI.setInspectedTabId",
                                             &tab_id, nullptr, nullptr);

  // Inherit owner window in devtools.
  if (owner_window())
    handle->SetOwnerWindow(managed_web_contents()->GetDevToolsWebContents(),
                           owner_window());
#endif

  EmitEvent("devtools-opened");
}

void MesonWebContentsBinding::DevToolsClosed() {
#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  devtools_web_contents_.Reset();
#endif

  EmitEvent("devtools-closed");
}

bool MesonWebContentsBinding::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MesonWebContentsBinding, message)
    IPC_MESSAGE_HANDLER(MesonViewHostMsg_Message, OnRendererMessage)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(MesoniewHostMsg_Message_Sync, OnRendererMessageSync)
#if 0
    IPC_MESSAGE_HANDLER_CODE(ViewHostMsg_SetCursor, OnCursorChange, handled = false)
#endif
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

// There are three ways of destroying a webContents:
// 1. call webContents.destroy();
// 2. garbage collection;
// 3. user closes the window of webContents;
// For webview only #1 will happen, for BrowserWindow both #1 and #3 may
// happen. The #2 should never happen for webContents, because webview is
// managed by GuestViewManager, and BrowserWindow's webContents is managed
// by api::Window.
// For #1, the destructor will do the cleanup work and we only need to make
// sure "destroyed" event is emitted. For #3, the content::WebContents will
// be destroyed on close, and WebContentsDestroyed would be called for it, so
// we need to make sure the api::WebContents is also deleted.
void MesonWebContentsBinding::WebContentsDestroyed() {
  // This event is only for internal use, which is emitted when WebContents is
  // being destroyed.
  EmitEvent("will-destroy");

// Cleanup relationships with other parts.
#if 0
  //TODO:
  RemoveFromWeakMap();

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  MarkDestroyed();
#endif

  EmitEvent("destroyed");

#if 0
  //TODO:
  // Destroy the native class in next tick.
  base::MessageLoop::current()->PostTask(FROM_HERE, GetDestroyClosure());
#endif
}

void MesonWebContentsBinding::NavigationEntryCommitted(const content::LoadCommittedDetails& details) {
  EmitEvent("navigation-entry-commited" /*TODO: , details.entry->GetURL(), details.is_in_page, details.did_replace_entry*/);
}

int64_t MesonWebContentsBinding::GetID() const {
  int64_t process_id = web_contents()->GetRenderProcessHost()->GetID();
  int64_t routing_id = web_contents()->GetRoutingID();
  int64_t rv = (process_id << 32) + routing_id;
  return rv;
}

int MesonWebContentsBinding::GetProcessID() const {
  return web_contents()->GetRenderProcessHost()->GetID();
}

MesonWebContentsBinding::Type MesonWebContentsBinding::GetType() const {
  return type_;
}

bool MesonWebContentsBinding::Equal(const MesonWebContentsBinding* web_contents) const {
  return GetID() == web_contents->GetID();
}

void MesonWebContentsBinding::LoadURL(const GURL& url, const base::DictionaryValue& options) {
  if (!url.is_valid()) {
    EmitEvent("did-fail-load", static_cast<int>(net::ERR_INVALID_URL), net::ErrorToShortString(net::ERR_INVALID_URL), url.possibly_invalid_spec(), true);
    return;
  }

  content::NavigationController::LoadURLParams params(url);

  std::string http_referrer_str;
  if (options.GetString("httpReferrer", &http_referrer_str)) {
    GURL http_referrer(http_referrer_str);
    params.referrer = content::Referrer(http_referrer.GetAsReferrer(), blink::WebReferrerPolicyDefault);
  }

  std::string user_agent;
  if (options.GetString("userAgent", &user_agent))
    web_contents()->SetUserAgentOverride(user_agent);

  std::string extra_headers;
  if (options.GetString("extraHeaders", &extra_headers))
    params.extra_headers = extra_headers;

  params.transition_type = ui::PAGE_TRANSITION_TYPED;
  params.should_clear_history_list = true;
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  web_contents()->GetController().LoadURLWithParams(params);

  // Set the background color of RenderWidgetHostView.
  // We have to call it right after LoadURL because the RenderViewHost is only
  // created after loading a page.
  const auto view = web_contents()->GetRenderWidgetHostView();
  WebContentsPreferences* web_preferences = WebContentsPreferences::FromWebContents(web_contents());
  std::string color_name;
  if (web_preferences->web_preferences()->GetString(options::kBackgroundColor, &color_name)) {
    view->SetBackgroundColor(ParseHexColor(color_name));
  } else {
    view->SetBackgroundColor(SK_ColorTRANSPARENT);
  }

  // For the same reason we can only disable hidden here.
  const auto host = static_cast<content::RenderWidgetHostImpl*>(view->GetRenderWidgetHost());
  host->disable_hidden_ = !background_throttling_;
}

void MesonWebContentsBinding::DownloadURL(const GURL& url) {
  auto browser_context = web_contents()->GetBrowserContext();
  auto download_manager = content::BrowserContext::GetDownloadManager(browser_context);

  download_manager->DownloadUrl(content::DownloadUrlParameters::CreateForWebContentsMainFrame(web_contents(), url));
}

GURL MesonWebContentsBinding::GetURL() const {
  return web_contents()->GetURL();
}

base::string16 MesonWebContentsBinding::GetTitle() const {
  return web_contents()->GetTitle();
}

bool MesonWebContentsBinding::IsLoading() const {
  return web_contents()->IsLoading();
}

bool MesonWebContentsBinding::IsLoadingMainFrame() const {
  // Comparing site instances works because Electron always creates a new site
  // instance when navigating, regardless of origin. See AtomBrowserClient.
  return (web_contents()->GetLastCommittedURL().is_empty() || web_contents()->GetSiteInstance() != web_contents()->GetPendingSiteInstance()) && IsLoading();
}

bool MesonWebContentsBinding::IsWaitingForResponse() const {
  return web_contents()->IsWaitingForResponse();
}

void MesonWebContentsBinding::Stop() {
  web_contents()->Stop();
}

void MesonWebContentsBinding::GoBack() {
  meson::MesonBrowserClient::SuppressRendererProcessRestartForOnce();
  web_contents()->GetController().GoBack();
}

void MesonWebContentsBinding::GoForward() {
  meson::MesonBrowserClient::SuppressRendererProcessRestartForOnce();
  web_contents()->GetController().GoForward();
}

void MesonWebContentsBinding::GoToOffset(int offset) {
  meson::MesonBrowserClient::SuppressRendererProcessRestartForOnce();
  web_contents()->GetController().GoToOffset(offset);
}

bool MesonWebContentsBinding::IsCrashed() const {
  return web_contents()->IsCrashed();
}

void MesonWebContentsBinding::SetUserAgent(const std::string& user_agent, base::ListValue* args) {
  web_contents()->SetUserAgentOverride(user_agent);
}

std::string MesonWebContentsBinding::GetUserAgent() {
  return web_contents()->GetUserAgentOverride();
}

void MesonWebContentsBinding::InsertCSS(const std::string& css) {
  web_contents()->InsertCSS(css);
}

#if 0
bool MesonWebContentsBinding::SavePage(const base::FilePath& full_file_path,
                                       const content::SavePageType& save_type,
                                       const SavePageHandler::SavePageCallback& callback) {
  auto handler = new SavePageHandler(web_contents(), callback);
  return handler->Handle(full_file_path, save_type);
}
#endif

void MesonWebContentsBinding::OpenDevTools(base::ListValue* args) {
  if (type_ == REMOTE)
    return;

  if (!enable_devtools_)
    return;

  std::string state;
  if (type_ == WEB_VIEW || !owner_window()) {
    state = "detach";
  } else if (args && args->GetSize() == 1) {
    bool detach = false;
    base::DictionaryValue* options = nullptr;
    if (args->GetDictionary(0, &options)) {
      options->GetString("mode", &state);

      // TODO(kevinsawicki) Remove in 2.0
      options->GetBoolean("detach", &detach);
      if (state.empty() && detach)
        state = "detach";
    }
  }
  managed_web_contents()->SetDockState(state);
  managed_web_contents()->ShowDevTools();
}

void MesonWebContentsBinding::CloseDevTools() {
  if (type_ == REMOTE)
    return;

  managed_web_contents()->CloseDevTools();
}

bool MesonWebContentsBinding::IsDevToolsOpened() {
  if (type_ == REMOTE)
    return false;

  return managed_web_contents()->IsDevToolsViewShowing();
}

bool MesonWebContentsBinding::IsDevToolsFocused() {
  if (type_ == REMOTE)
    return false;

  return managed_web_contents()->GetView()->IsDevToolsViewFocused();
}

#if 0
void MesonWebContentsBinding::EnableDeviceEmulation(const blink::WebDeviceEmulationParams& params) {
  if (type_ == REMOTE)
    return;

  Send(new ViewMsg_EnableDeviceEmulation(routing_id(), params));
}

void MesonWebContentsBinding::DisableDeviceEmulation() {
  if (type_ == REMOTE)
    return;

  Send(new ViewMsg_DisableDeviceEmulation(routing_id()));
}
#endif

void MesonWebContentsBinding::ToggleDevTools() {
  if (IsDevToolsOpened())
    CloseDevTools();
  else
    OpenDevTools(nullptr);
}

void MesonWebContentsBinding::InspectElement(int x, int y) {
  if (type_ == REMOTE)
    return;

  if (!enable_devtools_)
    return;

  if (!managed_web_contents()->GetDevToolsWebContents())
    OpenDevTools(nullptr);
  scoped_refptr<content::DevToolsAgentHost> agent(content::DevToolsAgentHost::GetOrCreateFor(web_contents()));
  agent->InspectElement(x, y);
}

void MesonWebContentsBinding::InspectServiceWorker() {
  if (type_ == REMOTE)
    return;

  if (!enable_devtools_)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() == content::DevToolsAgentHost::TYPE_SERVICE_WORKER) {
      OpenDevTools(nullptr);
      managed_web_contents()->AttachTo(agent_host);
      break;
    }
  }
}

void MesonWebContentsBinding::HasServiceWorker(const base::Callback<void(bool)>& callback) {
  LOG(ERROR) << __PRETTY_FUNCTION__ << " not implemented.";
#if 0
  //TODO:
  auto context = GetServiceWorkerContext(web_contents());
  if (!context)
    return;

  context->CheckHasServiceWorker(web_contents()->GetLastCommittedURL(), GURL::EmptyGURL(), callback);
#endif
}

void MesonWebContentsBinding::UnregisterServiceWorker(const base::Callback<void(bool)>& callback) {
  LOG(ERROR) << __PRETTY_FUNCTION__ << " not implemented.";
#if 0
  //TODO:
  auto context = GetServiceWorkerContext(web_contents());
  if (!context)
    return;

  context->UnregisterServiceWorker(web_contents()->GetLastCommittedURL(), callback);
#endif
}

void MesonWebContentsBinding::SetAudioMuted(bool muted) {
  web_contents()->SetAudioMuted(muted);
}

bool MesonWebContentsBinding::IsAudioMuted() {
  return web_contents()->IsAudioMuted();
}

#if 0
  //TODO:
void MesonWebContentsBinding::Print(mate::Arguments* args) {
  PrintSettings settings = {false, false};
  if (args->Length() == 1 && !args->GetNext(&settings)) {
    args->ThrowError();
    return;
  }

  printing::PrintViewManagerBasic::FromWebContents(web_contents())
      ->PrintNow(settings.silent, settings.print_background);
}

void MesonWebContentsBinding::PrintToPDF(const base::DictionaryValue& setting,
                                         const PrintToPDFCallback& callback) {
  printing::PrintPreviewMessageHandler::FromWebContents(web_contents())->PrintToPDF(setting, callback);
}
#endif

void MesonWebContentsBinding::AddWorkSpace(base::ListValue* args, const base::FilePath& path) {
//TODO: エラーを返す方法を検討する
#if 0
  if (path.empty()) {
    args->ThrowError("path cannot be empty");
    return;
  }
#endif
  DevToolsAddFileSystem(path);
}

void MesonWebContentsBinding::RemoveWorkSpace(base::ListValue* args, const base::FilePath& path) {
//TODO: エラーを返す方法を検討する
#if 0
  if (path.empty()) {
    args->ThrowError("path cannot be empty");
    return;
  }
#endif
  DevToolsRemoveFileSystem(path);
}

void MesonWebContentsBinding::Undo() {
  web_contents()->Undo();
}

void MesonWebContentsBinding::Redo() {
  web_contents()->Redo();
}

void MesonWebContentsBinding::Cut() {
  web_contents()->Cut();
}

void MesonWebContentsBinding::Copy() {
  web_contents()->Copy();
}

void MesonWebContentsBinding::Paste() {
  web_contents()->Paste();
}

void MesonWebContentsBinding::PasteAndMatchStyle() {
  web_contents()->PasteAndMatchStyle();
}

void MesonWebContentsBinding::Delete() {
  web_contents()->Delete();
}

void MesonWebContentsBinding::SelectAll() {
  web_contents()->SelectAll();
}

void MesonWebContentsBinding::Unselect() {
  web_contents()->Unselect();
}

void MesonWebContentsBinding::Replace(const base::string16& word) {
  web_contents()->Replace(word);
}

void MesonWebContentsBinding::ReplaceMisspelling(const base::string16& word) {
  web_contents()->ReplaceMisspelling(word);
}

uint32_t MesonWebContentsBinding::FindInPage(base::ListValue* args) {
#if 0
  //TODO:
  uint32_t request_id = GetNextRequestId();
  base::string16 search_text;
  blink::WebFindOptions options;
  if (!args->GetNext(&search_text) || search_text.empty()) {
    args->ThrowError("Must provide a non-empty search content");
    return 0;
  }

  args->GetNext(&options);

  web_contents()->Find(request_id, search_text, options);
  return request_id;
#else
  return 0;
#endif
}

void MesonWebContentsBinding::StopFindInPage(content::StopFindAction action) {
  web_contents()->StopFinding(action);
}

void MesonWebContentsBinding::ShowDefinitionForSelection() {
#if defined(OS_MACOSX)
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->ShowDefinitionForSelection();
#endif
}

void MesonWebContentsBinding::CopyImageAt(int x, int y) {
  const auto host = web_contents()->GetMainFrame();
  if (host)
    host->CopyImageAt(x, y);
}

void MesonWebContentsBinding::Focus() {
  web_contents()->Focus();
}

#if !defined(OS_MACOSX)
bool MesonWebContentsBinding::IsFocused() const {
  auto view = web_contents()->GetRenderWidgetHostView();
  if (!view)
    return false;

  if (GetType() != BACKGROUND_PAGE) {
    auto window = web_contents()->GetNativeView()->GetToplevelWindow();
    if (window && !window->IsVisible())
      return false;
  }

  return view->HasFocus();
}
#endif

void MesonWebContentsBinding::TabTraverse(bool reverse) {
  web_contents()->FocusThroughTabTraversal(reverse);
}

bool MesonWebContentsBinding::SendIPCMessage(bool all_frames, const base::string16& channel, const base::ListValue& args) {
  return Send(new MesonViewMsg_Message(routing_id(), all_frames, channel, args));
}

#if 0
void MesonWebContentsBinding::SendInputEvent(v8::Isolate* isolate, v8::Local<v8::Value> input_event) {
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (!view)
    return;
  const auto host = view->GetRenderWidgetHost();
  if (!host)
    return;

  int type = mate::GetWebInputEventType(isolate, input_event);
  if (blink::WebInputEvent::isMouseEventType(type)) {
    blink::WebMouseEvent mouse_event;
    if (mate::ConvertFromV8(isolate, input_event, &mouse_event)) {
      host->ForwardMouseEvent(mouse_event);
      return;
    }
  } else if (blink::WebInputEvent::isKeyboardEventType(type)) {
    content::NativeWebKeyboardEvent keyboard_event;
    if (mate::ConvertFromV8(isolate, input_event, &keyboard_event)) {
      host->ForwardKeyboardEvent(keyboard_event);
      return;
    }
  } else if (type == blink::WebInputEvent::MouseWheel) {
    blink::WebMouseWheelEvent mouse_wheel_event;
    if (mate::ConvertFromV8(isolate, input_event, &mouse_wheel_event)) {
      host->ForwardWheelEvent(mouse_wheel_event);
      return;
    }
  }

  isolate->ThrowException(
      v8::Exception::Error(mate::StringToV8(isolate, "Invalid event object")));
}
#endif

void MesonWebContentsBinding::BeginFrameSubscription(base::ListValue* args) {
#if 0
  //TODO:
  bool only_dirty = false;
  FrameSubscriber::FrameCaptureCallback callback;
  int args_idx = 0;

  args->GetBoolean(args_idx++, &only_dirty);
  if (!args->Get(args_idx++, &callback)) {
    args->ThrowError();
    return;
  }

  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view) {
    std::unique_ptr<FrameSubscriber> frame_subscriber(
        new FrameSubscriber(isolate(), view, callback, only_dirty));
    view->BeginFrameSubscription(std::move(frame_subscriber));
  }
#endif
}

void MesonWebContentsBinding::EndFrameSubscription() {
#if 0
  //TODO:
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->EndFrameSubscription();
#endif
}

void MesonWebContentsBinding::StartDrag(const base::DictionaryValue& item, base::ListValue* args) {
#if 0
  //TODO:
  base::FilePath file;
  std::vector<base::FilePath> files;
  if (!item.Get("files", &files) && item.Get("file", &file)) {
    files.push_back(file);
  }

  mate::Handle<NativeImage> icon;
  if (!item.Get("icon", &icon) && !file.empty()) {
    // TODO(zcbenz): Set default icon from file.
  }

  // Error checking.
  if (icon.IsEmpty()) {
    args->ThrowError("icon must be set");
    return;
  }

  // Start dragging.
  if (!files.empty()) {
    base::MessageLoop::ScopedNestableTaskAllower allow(
        base::MessageLoop::current());
    DragFileItems(files, icon->image(), web_contents()->GetNativeView());
  } else {
    args->ThrowError("There is nothing to drag");
  }
#endif
}

void MesonWebContentsBinding::CapturePage(base::ListValue* args) {
#if 0
  //TODO:
  gfx::Rect rect;
  base::Callback<void(const gfx::Image&)> callback;

  if (!(args->Length() == 1 && args->GetNext(&callback)) &&
      !(args->Length() == 2 && args->GetNext(&rect) && args->GetNext(&callback))) {
    args->ThrowError();
    return;
  }

  const auto view = web_contents()->GetRenderWidgetHostView();
  const auto host = view ? view->GetRenderWidgetHost() : nullptr;
  if (!view || !host) {
    callback.Run(gfx::Image());
    return;
  }

  // Capture full page if user doesn't specify a |rect|.
  const gfx::Size view_size = rect.IsEmpty() ? view->GetViewBounds().size() : rect.size();

  // By default, the requested bitmap size is the view size in screen
  // coordinates.  However, if there's more pixel detail available on the
  // current system, increase the requested bitmap size to capture it all.
  gfx::Size bitmap_size = view_size;
  const gfx::NativeView native_view = view->GetNativeView();
  const float scale = display::Screen::GetScreen()
                          ->GetDisplayNearestWindow(native_view)
                          .device_scale_factor();
  if (scale > 1.0f)
    bitmap_size = gfx::ScaleToCeiledSize(view_size, scale);

  host->CopyFromBackingStore(gfx::Rect(rect.origin(), view_size), bitmap_size,
                             base::Bind(&OnCapturePageDone, callback),
                             kBGRA_8888_SkColorType);
#endif
}

void MesonWebContentsBinding::OnCursorChange(const content::WebCursor& cursor) {
  content::WebCursor::CursorInfo info;
  cursor.GetCursorInfo(&info);

  if (cursor.IsCustom()) {
    EmitEvent("cursor-changed" /*, TODO:CursorTypeToString(info),
         gfx::Image::CreateFrom1xBitmap(info.custom_image),
         info.image_scale_factor,
         gfx::Size(info.custom_image.width(), info.custom_image.height()),
         info.hotspot */);
  } else {
    EmitEvent("cursor-changed" /*, TODO:CursorTypeToString(info)*/);
  }
}

void MesonWebContentsBinding::SetSize(const SetSizeParams& params) {
#if 0
  //TODO:
  if (guest_delegate_)
    guest_delegate_->SetSize(params);
#endif
}

void MesonWebContentsBinding::OnPaint(const gfx::Rect& dirty_rect, const SkBitmap& bitmap) {
#if 0
  //TODO:
  mate::Handle<NativeImage> image =      NativeImage::Create(isolate(), gfx::Image::CreateFrom1xBitmap(bitmap));
  Emit("paint", dirty_rect, image);
#endif
}

void MesonWebContentsBinding::StartPainting() {
  if (!IsOffscreen())
    return;
#if 0
  //TODO:

  auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  if (osr_rwhv)
    osr_rwhv->SetPainting(true);
#endif
}

void MesonWebContentsBinding::StopPainting() {
  if (!IsOffscreen())
    return;
#if 0
  //TODO:
  auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  if (osr_rwhv)
    osr_rwhv->SetPainting(false);
#endif
}

bool MesonWebContentsBinding::IsPainting() const {
  if (!IsOffscreen())
    return false;
#if 0
  //TODO:
  const auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  return osr_rwhv && osr_rwhv->IsPainting();
#endif
  return false;
}

void MesonWebContentsBinding::SetFrameRate(int frame_rate) {
  if (!IsOffscreen())
    return;
#if 0
  //TODO:
  auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  if (osr_rwhv)
    osr_rwhv->SetFrameRate(frame_rate);
#endif
}

int MesonWebContentsBinding::GetFrameRate() const {
  if (!IsOffscreen())
    return 0;
#if 0
  //TODO:
  const auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  return osr_rwhv ? osr_rwhv->GetFrameRate() : 0;
#endif
  return 0;
}

void MesonWebContentsBinding::Invalidate() {
  if (!IsOffscreen())
    return;
#if 0
  //TODO:
  auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  if (osr_rwhv)
    osr_rwhv->Invalidate();
#endif
}

#if 0
v8::Local<v8::Value> MesonWebContentsBinding::GetWebPreferences(v8::Isolate* isolate) {
  WebContentsPreferences* web_preferences =
      WebContentsPreferences::FromWebContents(web_contents());
  return mate::ConvertToV8(isolate, *web_preferences->web_preferences());
}

v8::Local<v8::Value> MesonWebContentsBinding::GetOwnerBrowserWindow() {
  if (owner_window())
    return Window::From(isolate(), owner_window());
  else
    return v8::Null(isolate());
}

int32_t MesonWebContentsBinding::ID() const {
  return weak_map_id();
}

v8::Local<v8::Value> MesonWebContentsBinding::Session(v8::Isolate* isolate) {
  return v8::Local<v8::Value>::New(isolate, session_);
}
#endif

content::WebContents* MesonWebContentsBinding::HostWebContents() {
  if (!embedder_)
    return nullptr;
  return embedder_->web_contents();
}

void MesonWebContentsBinding::SetEmbedder(const MesonWebContentsBinding* embedder) {
#if 0
  //TODO:
  if (embedder) {
    NativeWindow* owner_window = nullptr;
    auto relay = NativeWindowRelay::FromWebContents(embedder->web_contents());
    if (relay) {
      owner_window = relay->window.get();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);

    content::RenderWidgetHostView* rwhv =
        web_contents()->GetRenderWidgetHostView();
    if (rwhv) {
      rwhv->Hide();
      rwhv->Show();
    }
  }
#endif
}
#if 0
  //TODO:
v8::Local<v8::Value> MesonWebContentsBinding::DevToolsWebContents(v8::Isolate* isolate) {
  if (devtools_web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, devtools_web_contents_);
}

v8::Local<v8::Value> MesonWebContentsBinding::Debugger(v8::Isolate* isolate) {
  if (debugger_.IsEmpty()) {
    auto handle = atom::api::Debugger::Create(isolate, web_contents());
    debugger_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, debugger_);
}

// static
void MesonWebContentsBinding::BuildPrototype(v8::Isolate* isolate,
                                             v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "WebContents"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("getId", &MesonWebContentsBinding::GetID)
      .SetMethod("getProcessId", &MesonWebContentsBinding::GetProcessID)
      .SetMethod("equal", &MesonWebContentsBinding::Equal)
      .SetMethod("_loadURL", &MesonWebContentsBinding::LoadURL)
      .SetMethod("downloadURL", &MesonWebContentsBinding::DownloadURL)
      .SetMethod("_getURL", &MesonWebContentsBinding::GetURL)
      .SetMethod("getTitle", &MesonWebContentsBinding::GetTitle)
      .SetMethod("isLoading", &MesonWebContentsBinding::IsLoading)
      .SetMethod("isLoadingMainFrame", &MesonWebContentsBinding::IsLoadingMainFrame)
      .SetMethod("isWaitingForResponse", &MesonWebContentsBinding::IsWaitingForResponse)
      .SetMethod("_stop", &MesonWebContentsBinding::Stop)
      .SetMethod("_goBack", &MesonWebContentsBinding::GoBack)
      .SetMethod("_goForward", &MesonWebContentsBinding::GoForward)
      .SetMethod("_goToOffset", &MesonWebContentsBinding::GoToOffset)
      .SetMethod("isCrashed", &MesonWebContentsBinding::IsCrashed)
      .SetMethod("setUserAgent", &MesonWebContentsBinding::SetUserAgent)
      .SetMethod("getUserAgent", &MesonWebContentsBinding::GetUserAgent)
      .SetMethod("insertCSS", &MesonWebContentsBinding::InsertCSS)
      .SetMethod("savePage", &MesonWebContentsBinding::SavePage)
      .SetMethod("openDevTools", &MesonWebContentsBinding::OpenDevTools)
      .SetMethod("closeDevTools", &MesonWebContentsBinding::CloseDevTools)
      .SetMethod("isDevToolsOpened", &MesonWebContentsBinding::IsDevToolsOpened)
      .SetMethod("isDevToolsFocused", &MesonWebContentsBinding::IsDevToolsFocused)
      .SetMethod("enableDeviceEmulation", &MesonWebContentsBinding::EnableDeviceEmulation)
      .SetMethod("disableDeviceEmulation", &MesonWebContentsBinding::DisableDeviceEmulation)
      .SetMethod("toggleDevTools", &MesonWebContentsBinding::ToggleDevTools)
      .SetMethod("inspectElement", &MesonWebContentsBinding::InspectElement)
      .SetMethod("setAudioMuted", &MesonWebContentsBinding::SetAudioMuted)
      .SetMethod("isAudioMuted", &MesonWebContentsBinding::IsAudioMuted)
      .SetMethod("undo", &MesonWebContentsBinding::Undo)
      .SetMethod("redo", &MesonWebContentsBinding::Redo)
      .SetMethod("cut", &MesonWebContentsBinding::Cut)
      .SetMethod("copy", &MesonWebContentsBinding::Copy)
      .SetMethod("paste", &MesonWebContentsBinding::Paste)
      .SetMethod("pasteAndMatchStyle", &MesonWebContentsBinding::PasteAndMatchStyle)
      .SetMethod("delete", &MesonWebContentsBinding::Delete)
      .SetMethod("selectAll", &MesonWebContentsBinding::SelectAll)
      .SetMethod("unselect", &MesonWebContentsBinding::Unselect)
      .SetMethod("replace", &MesonWebContentsBinding::Replace)
      .SetMethod("replaceMisspelling", &MesonWebContentsBinding::ReplaceMisspelling)
      .SetMethod("findInPage", &MesonWebContentsBinding::FindInPage)
      .SetMethod("stopFindInPage", &MesonWebContentsBinding::StopFindInPage)
      .SetMethod("focus", &MesonWebContentsBinding::Focus)
      .SetMethod("isFocused", &MesonWebContentsBinding::IsFocused)
      .SetMethod("tabTraverse", &MesonWebContentsBinding::TabTraverse)
      .SetMethod("_send", &MesonWebContentsBinding::SendIPCMessage)
      .SetMethod("sendInputEvent", &MesonWebContentsBinding::SendInputEvent)
      .SetMethod("beginFrameSubscription", &MesonWebContentsBinding::BeginFrameSubscription)
      .SetMethod("endFrameSubscription", &MesonWebContentsBinding::EndFrameSubscription)
      .SetMethod("startDrag", &MesonWebContentsBinding::StartDrag)
      .SetMethod("setSize", &MesonWebContentsBinding::SetSize)
      .SetMethod("isGuest", &MesonWebContentsBinding::IsGuest)
      .SetMethod("isOffscreen", &MesonWebContentsBinding::IsOffScreen)
      .SetMethod("startPainting", &MesonWebContentsBinding::StartPainting)
      .SetMethod("stopPainting", &MesonWebContentsBinding::StopPainting)
      .SetMethod("isPainting", &MesonWebContentsBinding::IsPainting)
      .SetMethod("setFrameRate", &MesonWebContentsBinding::SetFrameRate)
      .SetMethod("getFrameRate", &MesonWebContentsBinding::GetFrameRate)
      .SetMethod("invalidate", &MesonWebContentsBinding::Invalidate)
      .SetMethod("getType", &MesonWebContentsBinding::GetType)
      .SetMethod("getWebPreferences", &MesonWebContentsBinding::GetWebPreferences)
      .SetMethod("getOwnerBrowserWindow", &MesonWebContentsBinding::GetOwnerBrowserWindow)
      .SetMethod("hasServiceWorker", &MesonWebContentsBinding::HasServiceWorker)
      .SetMethod("unregisterServiceWorker",
                 &MesonWebContentsBinding::UnregisterServiceWorker)
      .SetMethod("inspectServiceWorker", &MesonWebContentsBinding::InspectServiceWorker)
      .SetMethod("print", &MesonWebContentsBinding::Print)
      .SetMethod("_printToPDF", &MesonWebContentsBinding::PrintToPDF)
      .SetMethod("addWorkSpace", &MesonWebContentsBinding::AddWorkSpace)
      .SetMethod("removeWorkSpace", &MesonWebContentsBinding::RemoveWorkSpace)
      .SetMethod("showDefinitionForSelection",
                 &MesonWebContentsBinding::ShowDefinitionForSelection)
      .SetMethod("copyImageAt", &MesonWebContentsBinding::CopyImageAt)
      .SetMethod("capturePage", &MesonWebContentsBinding::CapturePage)
      .SetMethod("setEmbedder", &MesonWebContentsBinding::SetEmbedder)
      .SetProperty("id", &MesonWebContentsBinding::ID)
      .SetProperty("session", &MesonWebContentsBinding::Session)
      .SetProperty("hostWebContents", &MesonWebContentsBinding::HostWebContents)
      .SetProperty("devToolsWebContents", &MesonWebContentsBinding::DevToolsWebContents)
      .SetProperty("debugger", &MesonWebContentsBinding::Debugger);
}
#endif

void MesonWebContentsBinding::OnRendererMessage(const base::string16& channel, const base::ListValue& args) {
  // webContents.emit(channel, new Event(), args...);
  EmitEvent(base::UTF16ToUTF8(channel) /*, args*/);
}

void MesonWebContentsBinding::OnRendererMessageSync(const base::string16& channel, const base::ListValue& args, IPC::Message* message) {
  // webContents.emit(channel, new Event(sender, message), args...);
  //TODO:
  //EmitWithSender(base::UTF16ToUTF8(channel), web_contents(), message /*, args*/);
  EmitEvent(base::UTF16ToUTF8(channel), /*web_contents(), */message /*, args*/);
}

MesonWebContentsBindingFactory::MesonWebContentsBindingFactory(void) {
}
MesonWebContentsBindingFactory::~MesonWebContentsBindingFactory(void) {
}
APIBinding* MesonWebContentsBindingFactory::Create(unsigned int id, const api::APICreateArg& args) {
  return new MesonWebContentsBinding(id, std::move(args));
}
}
