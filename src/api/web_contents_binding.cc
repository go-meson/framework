#include "api/web_contents_binding.h"

#include "brightray/browser/inspectable_web_contents_view.h"

#include "api/session_binding.h"
#include "api/api.h"
#include "api/api_messages.h"
#include "browser/web_contents_permission_helper.h"
#include "browser/web_contents_preferences.h"
#include "browser/native_window.h"
#include "browser/browser_client.h"
#include "common/options_switches.h"
#include "common/color_util.h"
#include "common/mouse_util.h"
#include "content/public/common/context_menu_params.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/resource_request_details.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/download_manager.h"
#include "content/common/view_messages.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "third_party/WebKit/public/web/WebFindOptions.h"

#include "browser/web_view_manager.h"
#include "browser/web_view_guest_delegate.h"

namespace {
std::string WindowOpenDispositionToString(WindowOpenDisposition disposition) {
  std::string str_disposition = "other";
  switch (disposition) {
  case WindowOpenDisposition::CURRENT_TAB:
    str_disposition = "default";
    break;
  case WindowOpenDisposition::NEW_FOREGROUND_TAB:
    str_disposition = "foreground-tab";
    break;
  case WindowOpenDisposition::NEW_BACKGROUND_TAB:
    str_disposition = "background-tab";
    break;
  case WindowOpenDisposition::NEW_POPUP:
  case WindowOpenDisposition::NEW_WINDOW:
    str_disposition = "new-window";
    break;
  case WindowOpenDisposition::SAVE_TO_DISK:
    str_disposition = "save-to-disk";
    break;
  default:
    break;
  }
  return str_disposition;
}
}
namespace meson {
template <>
const APIBindingT<WebContentsBinding, WebContentsClassBinding>::MethodTable APIBindingT<WebContentsBinding, WebContentsClassBinding>::methodTable = {};
template <>
const APIClassBindingT<WebContentsBinding, WebContentsClassBinding>::MethodTable APIClassBindingT<WebContentsBinding, WebContentsClassBinding>::staticMethodTable = {
    {"_create", std::mem_fn(&WebContentsClassBinding::CreateInstance)},
};

MESON_IMPLEMENT_API_CLASS(WebContentsBinding, WebContentsClassBinding);

WebContentsBinding::WebContentsBinding(api::ObjID id, const base::DictionaryValue& args)
    : APIBindingT(MESON_OBJECT_TYPE_WEB_CONTENTS, id),
      embedder_(nullptr),
      type_(BROWSER_WINDOW),
      /*request_id_(0),*/ background_throttling_(true),
      enable_devtools_(true),
      guest_instance_id_(-1) {
  if (id == MESON_OBJID_STATIC) {
    return;
  }
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
    session_ = SessionBinding::Class().GetBinding(session_id);
  } else {
    base::DictionaryValue sessionArg;
    std::string partition;
    if (args.GetString("partition", &partition)) {
      sessionArg.SetString("partition", partition);
    }
    session_ = static_cast<SessionClassBinding&>(SessionBinding::Class()).NewInstance(sessionArg);
  }
  content::WebContents* web_contents;
  if (IsGuest()) {
    auto ctx = session_->GetSession();
    auto site_instance = content::SiteInstance::CreateForURL(ctx.get(), GURL("chrome-guest::/fake-host"));
    content::WebContents::CreateParams params(ctx.get(), site_instance);
    guest_delegate_.reset(new WebViewGuestDelegate);
    params.guest_delegate = guest_delegate_.get();
    web_contents = content::WebContents::Create(params);
    CHECK(args.GetInteger("guest_instance_id", &guest_instance_id_));
#if 0
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
#endif
  } else {
    content::WebContents::CreateParams params(session_->GetSession().get());
    web_contents = content::WebContents::Create(params);
  }

  //InitWithSessionAndOptions(isolate, web_contents, session, options);
  Observe(web_contents);
  InitWithWebContents(web_contents, session_->GetSession().get());

  managed_web_contents()->GetView()->SetDelegate(this);

  // Save the preferences in C++.
  new WebContentsPreferences(web_contents, args);

  // Intialize permission helper.
  WebContentsPermissionHelper::CreateForWebContents(web_contents);
  // Intialize security state client.
  SecurityStateTabHelper::CreateForWebContents(web_contents);

  web_contents->SetUserAgentOverride(GetBrowserContext()->GetUserAgent());

  if (IsGuest()) {
    guest_delegate_->Initialize(this);

    NativeWindow* owner_window = nullptr;
    int embed_id;
    if (args.GetInteger("embedder", &embed_id) && (embed_id != 0)) {
      //TODO: do embedder_ to WeakPtr?
      auto e = WebContentsBinding::Class().GetBinding(embed_id);
      embedder_ = base::AsWeakPtr(e.get());
      CHECK(embedder_) << "Embedder not found";
      // New WebContents's owner_window is the embedder's owner_window.
      auto relay = NativeWindowRelay::FromWebContents(embedder_->web_contents());
      if (relay)
        owner_window = relay->window.get();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);
  }
  //AttachAsUserData(web_contents);
}

WebContentsBinding::~WebContentsBinding(void) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << managed_web_contents();
  if (managed_web_contents()) {
    if (type_ == WEB_VIEW)
      guest_delegate_->Destroy();
    RenderViewDeleted(web_contents()->GetRenderViewHost());
    WebContentsDestroyedCore(true);
  }
}

MesonBrowserContext* WebContentsBinding::GetBrowserContext() const {
  return static_cast<MesonBrowserContext*>(web_contents()->GetBrowserContext());
}

bool WebContentsBinding::DidAddMessageToConsole(content::WebContents* source,
                                                int32_t level,
                                                const base::string16& message,
                                                int32_t line_no,
                                                const base::string16& source_id) {
  if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN) {
    return false;
  } else {
    EmitEvent("console-message",
              "level", level,
              "message", message,
              "lineNO", line_no,
              "sourceID", source_id);
    return true;
  }
}

void WebContentsBinding::OnCreateWindow(const GURL& target_url,
                                        const std::string& frame_name,
                                        WindowOpenDisposition disposition,
                                        const std::vector<std::string>& features) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << type_;
  if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN)
    EmitEvent("-new-window",
              "url", target_url,
              "frameName", frame_name,
              "disposition", WindowOpenDispositionToString(disposition),
              "features", features);
  else
    EmitEvent("new-window",
              "url", target_url,
              "frameName", frame_name,
              "disposition", WindowOpenDispositionToString(disposition),
              "features", features);
}

void WebContentsBinding::WebContentsCreated(content::WebContents* source_contents,
                                            int opener_render_process_id,
                                            int opener_render_frame_id,
                                            const std::string& frame_name,
                                            const GURL& target_url,
                                            content::WebContents* new_contents) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << target_url.spec();
#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto api_web_contents = CreateFrom(isolate(), new_contents, BROWSER_WINDOW);
#else
  auto api_web_contents = GetID();
#endif
  std::string url = target_url.spec();
  EmitEvent("-web-contents-created",
            "id", static_cast<int>(api_web_contents),
            "url", url,
            "frameName", frame_name);
}

void WebContentsBinding::AddNewContents(content::WebContents* source,
                                        content::WebContents* new_contents,
                                        WindowOpenDisposition disposition,
                                        const gfx::Rect& initial_rect,
                                        bool user_gesture,
                                        bool* was_blocked) {
  LOG(INFO) << __PRETTY_FUNCTION__;
#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto api_web_contents = CreateFrom(isolate(), new_contents);
#else
  int api_web_contents = GetID();
#endif
  EmitEvent("-add-new-contents",
            "id", api_web_contents,
            "disposition", WindowOpenDispositionToString(disposition),
            "userGesture", user_gesture,
            "initialRect", initial_rect);
}

content::WebContents* WebContentsBinding::OpenURLFromTab(content::WebContents* source,
                                                         const content::OpenURLParams& params) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (params.disposition != WindowOpenDisposition::CURRENT_TAB) {
    if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN)
      EmitEvent("-new-window",
                "url", params.url,
                "frameName", "",
                "disposition", WindowOpenDispositionToString(params.disposition));
    else
      EmitEvent("new-window",
                "url", params.url,
                "frameName", "",
                "disposition", WindowOpenDispositionToString(params.disposition));
    return nullptr;
  }

  // Give user a chance to cancel navigation.
  bool prevent = EmitPreventEvent("will-navigate", "url", params.url);
  if (prevent)
    return nullptr;

  return CommonWebContentsDelegate::OpenURLFromTab(source, params);
}

void WebContentsBinding::BeforeUnloadFired(content::WebContents* tab,
                                           bool proceed,
                                           bool* proceed_to_fire_unload) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (type_ == BROWSER_WINDOW || type_ == OFF_SCREEN)
    *proceed_to_fire_unload = proceed;
  else
    *proceed_to_fire_unload = true;
}

void WebContentsBinding::MoveContents(content::WebContents* source,
                                      const gfx::Rect& pos) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("move", "position", pos);
}

void WebContentsBinding::CloseContents(content::WebContents* source) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("close");

  if ((type_ == BROWSER_WINDOW || type_ == OFF_SCREEN) && owner_window()) {
    owner_window()->CloseContents(source);
  }
}

void WebContentsBinding::ActivateContents(content::WebContents* source) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("activate");
}

void WebContentsBinding::UpdateTargetURL(content::WebContents* source, const GURL& url) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("update-target-url", "url", url);
}

bool WebContentsBinding::IsPopupOrPanel(const content::WebContents* source) const {
  return type_ == BROWSER_WINDOW;
}

void WebContentsBinding::HandleKeyboardEvent(content::WebContents* source, const content::NativeWebKeyboardEvent& event) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (type_ == WEB_VIEW && embedder_) {
    // Send the unhandled keyboard events back to the embedder.
    embedder_->HandleKeyboardEvent(source, event);
  } else {
    // Go to the default keyboard handling.
    CommonWebContentsDelegate::HandleKeyboardEvent(source, event);
  }
}

void WebContentsBinding::EnterFullscreenModeForTab(content::WebContents* source, const GURL& origin) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  auto permission_helper = WebContentsPermissionHelper::FromWebContents(source);
  auto callback = base::Bind(&WebContentsBinding::OnEnterFullscreenModeForTab, base::Unretained(this), source, origin);
  permission_helper->RequestFullscreenPermission(callback);
}

void WebContentsBinding::OnEnterFullscreenModeForTab(content::WebContents* source, const GURL& origin, bool allowed) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (!allowed)
    return;
  CommonWebContentsDelegate::EnterFullscreenModeForTab(source, origin);
  EmitEvent("enter-html-full-screen");
}

void WebContentsBinding::ExitFullscreenModeForTab(content::WebContents* source) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  CommonWebContentsDelegate::ExitFullscreenModeForTab(source);
  EmitEvent("leave-html-full-screen");
}

void WebContentsBinding::RendererUnresponsive(content::WebContents* source, const content::WebContentsUnresponsiveState& unresponsive_state) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("unresponsive");
  if ((type_ == BROWSER_WINDOW || type_ == OFF_SCREEN) && owner_window())
    owner_window()->RendererUnresponsive(source);
}

void WebContentsBinding::RendererResponsive(content::WebContents* source) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("responsive");
  if ((type_ == BROWSER_WINDOW || type_ == OFF_SCREEN) && owner_window())
    owner_window()->RendererResponsive(source);
}

namespace {
inline base::DictionaryValue* ToDict(const content::ContextMenuParams& params) {
  std::unique_ptr<base::DictionaryValue> d(new base::DictionaryValue());
  d->SetInteger("x", params.x);
  d->SetInteger("y", params.y);
  d->SetString("linkURL", params.link_url.spec());
  d->SetString("linkText", params.link_text);
  d->SetString("pageURL", params.page_url.spec());
  d->SetString("frameURL", params.frame_url.spec());
  d->SetString("srcURL", params.src_url.spec());
  std::string t;
  switch (params.media_type) {
    case blink::WebContextMenuData::MediaType::MediaTypeImage:
      t = "image";
    case blink::WebContextMenuData::MediaType::MediaTypeAudio:
      t = "audio";
      break;
    case blink::WebContextMenuData::MediaType::MediaTypeVideo:
      t = "video";
      break;
    case blink::WebContextMenuData::MediaType::MediaTypeCanvas:
      t = "canvas";
      break;
    case blink::WebContextMenuData::MediaType::MediaTypeFile:
      t = "file";
      break;
    case blink::WebContextMenuData::MediaType::MediaTypePlugin:
      t = "plugin";
      break;
    case blink::WebContextMenuData::MediaType::MediaTypeNone:
      t = "none";
      break;
  }
  d->SetString("mediaType", t);
  d->SetBoolean("hasImageContents", params.has_image_contents);
  d->SetBoolean("isEditable", params.is_editable);
  d->SetString("selectionText", params.selection_text);
  d->SetString("titleText", params.title_text);
  d->SetString("missSpelledWord", params.misspelled_word);
  d->SetString("frameCharset", params.frame_charset);
  switch (params.input_field_type) {
    case blink::WebContextMenuData::InputFieldType::InputFieldTypeNone:
      t = "none";
      break;
    case blink::WebContextMenuData::InputFieldType::InputFieldTypePlainText:
      t = "plainText";
      break;
    case blink::WebContextMenuData::InputFieldType::InputFieldTypePassword:
      t = "password";
      break;
    case blink::WebContextMenuData::InputFieldType::InputFieldTypeOther:
      t = "other";
      break;
  }
  d->SetString("inputFieldType", t);

  switch (params.source_type) {
    case ui::MENU_SOURCE_NONE:
      t = "none";
      break;
    case ui::MENU_SOURCE_MOUSE:
      t = "mouse";
      break;
    case ui::MENU_SOURCE_KEYBOARD:
      t = "keyboard";
      break;
    case ui::MENU_SOURCE_TOUCH:
      t = "touch";
      break;
    case ui::MENU_SOURCE_TOUCH_EDIT_MENU:
      t = "touchMenu";
      break;
    case ui::MENU_SOURCE_LONG_PRESS:
      t = "longPress";
      break;
    case ui::MENU_SOURCE_LONG_TAP:
      t = "longTap";
      break;
  }
  d->SetString("menuSourceType", t);

  std::unique_ptr<base::DictionaryValue> dd(new base::DictionaryValue());
  dd->SetBoolean("inError", params.media_flags & blink::WebContextMenuData::MediaFlags::MediaInError);
  dd->SetBoolean("inPaused", params.media_flags & blink::WebContextMenuData::MediaFlags::MediaPaused);
  dd->SetBoolean("inMuted", params.media_flags & blink::WebContextMenuData::MediaFlags::MediaMuted);
  dd->SetBoolean("hasAudio", params.media_flags & blink::WebContextMenuData::MediaFlags::MediaHasAudio);
  dd->SetBoolean("isLooping", params.media_flags & blink::WebContextMenuData::MediaFlags::MediaLoop);
  dd->SetBoolean("isControlsVisible", params.media_flags & blink::WebContextMenuData::MediaFlags::MediaControls);
  dd->SetBoolean("canToggleControls", params.media_flags & blink::WebContextMenuData::MediaFlags::MediaCanToggleControls);
  dd->SetBoolean("canRemote", params.media_flags & blink::WebContextMenuData::MediaFlags::MediaCanRotate);
  d->Set("mediaFlags", std::move(dd));

  dd.reset(new base::DictionaryValue());
  dd->SetBoolean("canUndo", params.edit_flags & blink::WebContextMenuData::EditFlags::CanUndo);
  dd->SetBoolean("canRedo", params.edit_flags & blink::WebContextMenuData::EditFlags::CanRedo);
  dd->SetBoolean("canCut", params.edit_flags & blink::WebContextMenuData::EditFlags::CanCut);
  dd->SetBoolean("canCopy", params.edit_flags & blink::WebContextMenuData::EditFlags::CanCopy);
  dd->SetBoolean("canPaste", params.edit_flags & blink::WebContextMenuData::EditFlags::CanPaste);
  dd->SetBoolean("canDelete", params.edit_flags & blink::WebContextMenuData::EditFlags::CanDelete);
  dd->SetBoolean("canSelectAll", params.edit_flags & blink::WebContextMenuData::EditFlags::CanSelectAll);
  d->Set("editFlags", std::move(d));

  return d.release();
}
}

bool WebContentsBinding::HandleContextMenu(const content::ContextMenuParams& params) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  std::unique_ptr<base::DictionaryValue> p(ToDict(params));

  if (params.custom_context.is_pepper_menu) {
    //TODO: pass web_contents?

    EmitEvent("pepper-context-menu", "params", p.get());
    web_contents()->NotifyContextMenuClosed(params.custom_context);
  } else {
    //TODO: pass web_contents?
    EmitEvent("context-menu", "params", p.get());
  }

  return true;
}

bool WebContentsBinding::OnGoToEntryOffset(int offset) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  GoToOffset(offset);
  return false;
}

void WebContentsBinding::FindReply(content::WebContents* web_contents,
                                   int request_id,
                                   int number_of_matches,
                                   const gfx::Rect& selection_rect,
                                   int active_match_ordinal,
                                   bool final_update) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (!final_update)
    return;

  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  result->SetInteger("requestId", request_id);
  result->SetInteger("matches", number_of_matches);
  std::unique_ptr<base::DictionaryValue> d(new base::DictionaryValue());
  d->SetInteger("x", selection_rect.x());
  d->SetInteger("y", selection_rect.y());
  d->SetInteger("width", selection_rect.width());
  d->SetInteger("height", selection_rect.height());
  result->Set("selectionArea", std::move(d));
  result->SetInteger("activeMatchOrdinal", active_match_ordinal);
  result->SetBoolean("finalUpdate", final_update);  // Deprecate after 2.0
  EmitEvent("found-in-page", "result", result.get());
}

bool WebContentsBinding::CheckMediaAccessPermission(content::WebContents* web_contents, const GURL& security_origin, content::MediaStreamType type) {
  return true;
}

void WebContentsBinding::RequestMediaAccessPermission(content::WebContents* web_contents,
                                                      const content::MediaStreamRequest& request,
                                                      const content::MediaResponseCallback& callback) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  auto permission_helper = WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestMediaAccessPermission(request, callback);
}

void WebContentsBinding::RequestToLockMouse(content::WebContents* web_contents, bool user_gesture, bool last_unlocked_by_target) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  auto permission_helper = WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestPointerLockPermission(user_gesture);
}

std::unique_ptr<content::BluetoothChooser> WebContentsBinding::RunBluetoothChooser(content::RenderFrameHost* frame, const content::BluetoothChooser::EventHandler& event_handler) {
  LOG(INFO) << __PRETTY_FUNCTION__;
#if 0
  //TODO:
  std::unique_ptr<BluetoothChooser> bluetooth_chooser(new BluetoothChooser(this, event_handler));
  return std::move(bluetooth_chooser);
#else
  return nullptr;
#endif
}

void WebContentsBinding::BeforeUnloadFired(const base::TimeTicks& proceed_time) {
  // Do nothing, we override this method just to avoid compilation error since
  // there are two virtual functions named BeforeUnloadFired.
}

void WebContentsBinding::RenderViewDeleted(content::RenderViewHost* render_view_host) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("render-view-deleted", "id", render_view_host->GetProcess()->GetID());
}

void WebContentsBinding::RenderProcessGone(base::TerminationStatus status) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("crashed", "killed", status == base::TERMINATION_STATUS_PROCESS_WAS_KILLED);
}

void WebContentsBinding::PluginCrashed(const base::FilePath& plugin_path, base::ProcessId plugin_pid) {
  //TODO:
  LOG(INFO) << __PRETTY_FUNCTION__;
  content::WebPluginInfo info;
  auto plugin_service = content::PluginService::GetInstance();
  plugin_service->GetPluginInfoByPath(plugin_path, &info);
  EmitEvent("plugin-crashed", "name", info.name, "version", info.version);
}

void WebContentsBinding::MediaStartedPlaying(const MediaPlayerInfo& video_type, const MediaPlayerId& id) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("media-started-playing");
}

void WebContentsBinding::MediaStoppedPlaying(const MediaPlayerInfo& video_type, const MediaPlayerId& id) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("media-paused");
}

void WebContentsBinding::DidChangeThemeColor(SkColor theme_color) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("did-change-theme-color", "color", meson::ToRGBHex(theme_color));
}

void WebContentsBinding::DocumentLoadedInFrame(content::RenderFrameHost* render_frame_host) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "[" << guest_instance_id_ << "]";
  if (!render_frame_host->GetParent())
    EmitEvent("dom-ready");
}

void WebContentsBinding::DidFinishLoad(content::RenderFrameHost* render_frame_host, const GURL& validated_url) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << guest_instance_id_;
  bool is_main_frame = !render_frame_host->GetParent();
  EmitEvent("did-frame-finish-load", "isMainFrame", is_main_frame);

  if (is_main_frame)
    EmitEvent("did-finish-load");
}

void WebContentsBinding::DidFailLoad(content::RenderFrameHost* render_frame_host,
                                     const GURL& url,
                                     int error_code,
                                     const base::string16& error_description,
                                     bool was_ignored_by_handler) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  bool is_main_frame = !render_frame_host->GetParent();
  EmitEvent("did-fail-load",
            "errorCode", error_code,
            "errorDescription", error_description,
            "validateURL", url,
            "isMainFrame", is_main_frame);
}

void WebContentsBinding::DidStartLoading() {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("did-start-loading");
}

void WebContentsBinding::DidStopLoading() {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("did-stop-loading");
}

namespace {
inline base::DictionaryValue* ToDict(scoped_refptr<net::HttpResponseHeaders> headers) {
  std::unique_ptr<base::DictionaryValue> d(new base::DictionaryValue());
  if (headers) {
    size_t iter = 0;
    std::string n, v;
    while (headers->EnumerateHeaderLines(&iter, &n, &v)) {
      d->SetString(n, v);
    }
  }
  return d.release();
}
}

void WebContentsBinding::DidGetResourceResponseStart(const content::ResourceRequestDetails& details) {
  std::unique_ptr<base::DictionaryValue> h(ToDict(details.headers));
  EmitEvent("did-get-response-details",
            "status", details.socket_address.IsEmpty(),
            "newURL", details.url,
            "originalURL", details.original_url,
            "httpResponseCode", details.http_response_code,
            "requestMethod", details.method,
            "referer", details.referrer,
            "headers", h.get(),
            "resourceType", details.resource_type);
}

void WebContentsBinding::DidGetRedirectForResourceRequest(const content::ResourceRedirectDetails& details) {
  std::unique_ptr<base::DictionaryValue> h(ToDict(details.headers));
  EmitEvent("did-get-redirect-request",
            "oldURL", details.url,
            "newURL", details.new_url,
            "isMainFrame", (details.resource_type == content::RESOURCE_TYPE_MAIN_FRAME),
            "httpResponseCode", details.http_response_code,
            "requestMethod", details.method,
            "referer", details.referrer,
            "headers", h.get());
}

void WebContentsBinding::DidFinishNavigation(content::NavigationHandle* navigation_handle) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  bool is_main_frame = navigation_handle->IsInMainFrame();
  if (navigation_handle->HasCommitted() && !navigation_handle->IsErrorPage()) {
    auto url = navigation_handle->GetURL();
    bool is_in_page = navigation_handle->IsSamePage();
    if (is_main_frame && !is_in_page) {
      EmitEvent("did-navigate", "url", url);
    } else if (is_in_page) {
      EmitEvent("did-navigate-in-page", "url", url, "isMainFrame", is_main_frame);
    }
  } else {
    auto url = navigation_handle->GetURL();
    int code = navigation_handle->GetNetErrorCode();
    auto description = net::ErrorToShortString(code);
    EmitEvent("did-fail-provisional-load",
              "errorCode", code,
              "errorDescription", description,
              "validatedURL", url,
              "isMainFrame", is_main_frame);

    // Do not emit "did-fail-load" for canceled requests.
    if (code != net::ERR_ABORTED)
      EmitEvent("did-fail-load",
                "errorCode", code,
                "errorDescription", description,
                "validatedURL", url,
                "isMainFrame", is_main_frame);
  }
}

void WebContentsBinding::TitleWasSet(content::NavigationEntry* entry, bool explicit_set) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (entry)
    EmitEvent("-page-title-updated", "title", entry->GetTitle(), "explicitSet", explicit_set);
  else
    EmitEvent("-page-title-updated", "title", "", "explicitSet", explicit_set);
}

void WebContentsBinding::DidUpdateFaviconURL(const std::vector<content::FaviconURL>& urls) {
  std::set<GURL> unique_urls;
  for (const auto& iter : urls) {
    if (iter.icon_type != content::FaviconURL::FAVICON)
      continue;
    const GURL& url = iter.icon_url;
    if (url.is_valid())
      unique_urls.insert(url);
  }
  std::vector<std::string> url_strs;
  for (const auto& url : unique_urls) {
    url_strs.push_back(url.spec());
  }
  EmitEvent("page-favicon-updated", "favicons", url_strs);
}

void WebContentsBinding::DevToolsReloadPage() {
  EmitEvent("devtools-reload-page");
}

void WebContentsBinding::DevToolsFocused() {
  EmitEvent("devtools-focused");
}

void WebContentsBinding::DevToolsOpened() {
//TODO:
#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto handle = WebContentsBinding::CreateFrom(isolate(), managed_web_contents()->GetDevToolsWebContents());
  auto c = managed_web_contents()->GetDevToolsWebContents();
  auto handle = API::Get()->Create(WebContentsBinding::Name, c);
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

void WebContentsBinding::DevToolsClosed() {
#if 0
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  devtools_web_contents_.Reset();
#endif
  devtools_web_contents_ = nullptr;

  EmitEvent("devtools-closed");
}

bool WebContentsBinding::OnMessageReceived(const IPC::Message& message) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << "[" << guest_instance_id_ << "] : " << message.type() << " : " << message.size();
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebContentsBinding, message)
    IPC_MESSAGE_HANDLER(MesonViewHostMsg_Message, OnRendererMessage)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(MesonViewHostMsg_Message_Sync, OnRendererMessageSync)

    IPC_MESSAGE_HANDLER_CODE(ViewHostMsg_SetCursor, OnCursorChange, handled = false)
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
void WebContentsBinding::WebContentsDestroyed() {
  LOG(INFO) << __PRETTY_FUNCTION__;
  WebContentsDestroyedCore(false);
}

void WebContentsBinding::WebContentsDestroyedCore(bool destructor) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << destructor;
  // This event is only for internal use, which is emitted when WebContents is
  // being destroyed.
  EmitEvent("will-destroy");

  // Cleanup relationships with other parts.
  if (!destructor) {
    auto self = WebContentsBinding::Class().GetBinding(GetID());
    if (self) {
      CHECK(self.get() == this);
      WebContentsBinding::Class().RemoveBinding(this);
    }
  }

  EmitEvent("destroyed");
}

void WebContentsBinding::NavigationEntryCommitted(const content::LoadCommittedDetails& details) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  EmitEvent("navigation-entry-commited",
            "url", details.entry->GetURL(),
            "isInPage", details.is_in_page,
            "didReplaceEntry", details.did_replace_entry);
}

int64_t WebContentsBinding::GetWebContentsID() const {
  int64_t process_id = web_contents()->GetRenderProcessHost()->GetID();
  int64_t routing_id = web_contents()->GetRoutingID();
  int64_t rv = (process_id << 32) + routing_id;
  return rv;
}

int WebContentsBinding::GetProcessID() const {
  return web_contents()->GetRenderProcessHost()->GetID();
}

WebContentsBinding::Type WebContentsBinding::GetType() const {
  return type_;
}

bool WebContentsBinding::Equal(const WebContentsBinding* web_contents) const {
  return GetWebContentsID() == web_contents->GetWebContentsID();
}

void WebContentsBinding::LoadURL(const GURL& url, const base::DictionaryValue& options) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (!url.is_valid()) {
    EmitEvent("did-fail-load",
              "errorCode", static_cast<int>(net::ERR_INVALID_URL),
              "errorDescription", net::ErrorToShortString(net::ERR_INVALID_URL),
              "validatedURL", url.possibly_invalid_spec(),
              "isMainFrame", true);
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

void WebContentsBinding::DownloadURL(const GURL& url) {
  auto browser_context = web_contents()->GetBrowserContext();
  auto download_manager = content::BrowserContext::GetDownloadManager(browser_context);

  download_manager->DownloadUrl(content::DownloadUrlParameters::CreateForWebContentsMainFrame(web_contents(), url));
}

GURL WebContentsBinding::GetURL() const {
  return web_contents()->GetURL();
}

base::string16 WebContentsBinding::GetTitle() const {
  return web_contents()->GetTitle();
}

bool WebContentsBinding::IsLoading() const {
  return web_contents()->IsLoading();
}

bool WebContentsBinding::IsLoadingMainFrame() const {
  // Comparing site instances works because Electron always creates a new site
  // instance when navigating, regardless of origin. See AtomBrowserClient.
  return (web_contents()->GetLastCommittedURL().is_empty() || web_contents()->GetSiteInstance() != web_contents()->GetPendingSiteInstance()) && IsLoading();
}

bool WebContentsBinding::IsWaitingForResponse() const {
  return web_contents()->IsWaitingForResponse();
}

void WebContentsBinding::Stop() {
  web_contents()->Stop();
}

void WebContentsBinding::GoBack() {
  MesonBrowserClient::SuppressRendererProcessRestartForOnce();
  web_contents()->GetController().GoBack();
}

void WebContentsBinding::GoForward() {
  MesonBrowserClient::SuppressRendererProcessRestartForOnce();
  web_contents()->GetController().GoForward();
}

void WebContentsBinding::GoToOffset(int offset) {
  MesonBrowserClient::SuppressRendererProcessRestartForOnce();
  web_contents()->GetController().GoToOffset(offset);
}

bool WebContentsBinding::IsCrashed() const {
  return web_contents()->IsCrashed();
}

void WebContentsBinding::SetUserAgent(const std::string& user_agent, base::ListValue* args) {
  web_contents()->SetUserAgentOverride(user_agent);
}

std::string WebContentsBinding::GetUserAgent() {
  return web_contents()->GetUserAgentOverride();
}

#if 0
bool WebContentsBinding::SavePage(const base::FilePath& full_file_path,
                                       const content::SavePageType& save_type,
                                       const SavePageHandler::SavePageCallback& callback) {
  auto handler = new SavePageHandler(web_contents(), callback);
  return handler->Handle(full_file_path, save_type);
}
#endif

void WebContentsBinding::OpenDevTools(base::ListValue* args) {
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

void WebContentsBinding::CloseDevTools() {
  if (type_ == REMOTE)
    return;

  managed_web_contents()->CloseDevTools();
}

bool WebContentsBinding::IsDevToolsOpened() {
  if (type_ == REMOTE)
    return false;

  return managed_web_contents()->IsDevToolsViewShowing();
}

bool WebContentsBinding::IsDevToolsFocused() {
  if (type_ == REMOTE)
    return false;

  return managed_web_contents()->GetView()->IsDevToolsViewFocused();
}

#if 0
void WebContentsBinding::EnableDeviceEmulation(const blink::WebDeviceEmulationParams& params) {
  if (type_ == REMOTE)
    return;

  Send(new ViewMsg_EnableDeviceEmulation(routing_id(), params));
}

void WebContentsBinding::DisableDeviceEmulation() {
  if (type_ == REMOTE)
    return;

  Send(new ViewMsg_DisableDeviceEmulation(routing_id()));
}
#endif

void WebContentsBinding::ToggleDevTools() {
  if (IsDevToolsOpened())
    CloseDevTools();
  else
    OpenDevTools(nullptr);
}

void WebContentsBinding::InspectElement(int x, int y) {
  if (type_ == REMOTE)
    return;

  if (!enable_devtools_)
    return;

  if (!managed_web_contents()->GetDevToolsWebContents())
    OpenDevTools(nullptr);
  managed_web_contents()->InspectElement(x, y);
}

void WebContentsBinding::InspectServiceWorker() {
  if (type_ == REMOTE)
    return;

  if (!enable_devtools_)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() == content::DevToolsAgentHost::kTypeServiceWorker) {
      OpenDevTools(nullptr);
      managed_web_contents()->AttachTo(agent_host);
      break;
    }
  }
}

void WebContentsBinding::HasServiceWorker(const base::Callback<void(bool)>& callback) {
  LOG(ERROR) << __PRETTY_FUNCTION__ << " not implemented.";
#if 0
  //TODO:
  auto context = GetServiceWorkerContext(web_contents());
  if (!context)
    return;

  context->CheckHasServiceWorker(web_contents()->GetLastCommittedURL(), GURL::EmptyGURL(), callback);
#endif
}

void WebContentsBinding::UnregisterServiceWorker(const base::Callback<void(bool)>& callback) {
  LOG(ERROR) << __PRETTY_FUNCTION__ << " not implemented.";
#if 0
  //TODO:
  auto context = GetServiceWorkerContext(web_contents());
  if (!context)
    return;

  context->UnregisterServiceWorker(web_contents()->GetLastCommittedURL(), callback);
#endif
}

void WebContentsBinding::SetAudioMuted(bool muted) {
  web_contents()->SetAudioMuted(muted);
}

bool WebContentsBinding::IsAudioMuted() {
  return web_contents()->IsAudioMuted();
}

#if 0
  //TODO:
void WebContentsBinding::Print(mate::Arguments* args) {
  PrintSettings settings = {false, false};
  if (args->Length() == 1 && !args->GetNext(&settings)) {
    args->ThrowError();
    return;
  }

  printing::PrintViewManagerBasic::FromWebContents(web_contents())
      ->PrintNow(settings.silent, settings.print_background);
}

void WebContentsBinding::PrintToPDF(const base::DictionaryValue& setting,
                                         const PrintToPDFCallback& callback) {
  printing::PrintPreviewMessageHandler::FromWebContents(web_contents())->PrintToPDF(setting, callback);
}
#endif

void WebContentsBinding::AddWorkSpace(base::ListValue* args, const base::FilePath& path) {
  LOG(INFO) << __PRETTY_FUNCTION__;
//TODO: エラーを返す方法を検討する
#if 0
  if (path.empty()) {
    args->ThrowError("path cannot be empty");
    return;
  }
#endif
  DevToolsAddFileSystem(path);
}

void WebContentsBinding::RemoveWorkSpace(base::ListValue* args, const base::FilePath& path) {
  LOG(INFO) << __PRETTY_FUNCTION__;
//TODO: エラーを返す方法を検討する
#if 0
  if (path.empty()) {
    args->ThrowError("path cannot be empty");
    return;
  }
#endif
  DevToolsRemoveFileSystem(path);
}

void WebContentsBinding::Undo() {
  web_contents()->Undo();
}

void WebContentsBinding::Redo() {
  web_contents()->Redo();
}

void WebContentsBinding::Cut() {
  web_contents()->Cut();
}

void WebContentsBinding::Copy() {
  web_contents()->Copy();
}

void WebContentsBinding::Paste() {
  web_contents()->Paste();
}

void WebContentsBinding::PasteAndMatchStyle() {
  web_contents()->PasteAndMatchStyle();
}

void WebContentsBinding::Delete() {
  web_contents()->Delete();
}

void WebContentsBinding::SelectAll() {
  web_contents()->SelectAll();
}

void WebContentsBinding::Unselect() {
  web_contents()->Unselect();
}

void WebContentsBinding::Replace(const base::string16& word) {
  web_contents()->Replace(word);
}

void WebContentsBinding::ReplaceMisspelling(const base::string16& word) {
  web_contents()->ReplaceMisspelling(word);
}

size_t WebContentsBinding::FindInPage(base::ListValue* args) {
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

void WebContentsBinding::StopFindInPage(content::StopFindAction action) {
  web_contents()->StopFinding(action);
}

void WebContentsBinding::ShowDefinitionForSelection() {
#if defined(OS_MACOSX)
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->ShowDefinitionForSelection();
#endif
}

void WebContentsBinding::CopyImageAt(int x, int y) {
  const auto host = web_contents()->GetMainFrame();
  if (host)
    host->CopyImageAt(x, y);
}

void WebContentsBinding::Focus() {
  web_contents()->Focus();
}

#if !defined(OS_MACOSX)
bool WebContentsBinding::IsFocused() const {
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

void WebContentsBinding::TabTraverse(bool reverse) {
  web_contents()->FocusThroughTabTraversal(reverse);
}

bool WebContentsBinding::SendIPCMessage(bool all_frames, const base::string16& channel, const base::ListValue& args) {
  return Send(new MesonViewMsg_Message(routing_id(), all_frames, channel, args));
}

#if 0
void WebContentsBinding::SendInputEvent(v8::Isolate* isolate, v8::Local<v8::Value> input_event) {
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

void WebContentsBinding::BeginFrameSubscription(base::ListValue* args) {
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

void WebContentsBinding::EndFrameSubscription() {
#if 0
  //TODO:
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->EndFrameSubscription();
#endif
}

void WebContentsBinding::StartDrag(const base::DictionaryValue& item, base::ListValue* args) {
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

void WebContentsBinding::CapturePage(base::ListValue* args) {
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

void WebContentsBinding::OnCursorChange(const content::WebCursor& cursor) {
  content::WebCursor::CursorInfo info;
  cursor.GetCursorInfo(&info);

  if (cursor.IsCustom()) {
    EmitEvent("cursor-changed", "type", CursorTypeToString(info)
              /*,TODO: nativeImage      gfx::Image::CreateFrom1xBitmap(info.custom_image),
         info.image_scale_factor,
         gfx::Size(info.custom_image.width(), info.custom_image.height()),
         info.hotspot */);
  } else {
    EmitEvent("cursor-changed", "type", CursorTypeToString(info));
  }
}

void WebContentsBinding::SetSize(const SetSizeParams& params) {
  if (guest_delegate_)
    guest_delegate_->SetSize(params);
}

void WebContentsBinding::OnPaint(const gfx::Rect& dirty_rect, const SkBitmap& bitmap) {
#if 0
  //TODO: nativeImage
  mate::Handle<NativeImage> image =      NativeImage::Create(isolate(), gfx::Image::CreateFrom1xBitmap(bitmap));
  Emit("paint", dirty_rect, image);
#endif
}

void WebContentsBinding::StartPainting() {
  if (!IsOffscreen())
    return;
#if 0
  //TODO: offscreen support

  auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  if (osr_rwhv)
    osr_rwhv->SetPainting(true);
#endif
}

void WebContentsBinding::StopPainting() {
  if (!IsOffscreen())
    return;
#if 0
  //TODO: offscreen support
  auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  if (osr_rwhv)
    osr_rwhv->SetPainting(false);
#endif
}

bool WebContentsBinding::IsPainting() const {
  if (!IsOffscreen())
    return false;
#if 0
  //TODO: offscreen support
  const auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  return osr_rwhv && osr_rwhv->IsPainting();
#endif
  return false;
}

void WebContentsBinding::SetFrameRate(int frame_rate) {
  if (!IsOffscreen())
    return;
#if 0
  //TODO: offscreen support
  auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  if (osr_rwhv)
    osr_rwhv->SetFrameRate(frame_rate);
#endif
}

int WebContentsBinding::GetFrameRate() const {
  if (!IsOffscreen())
    return 0;
#if 0
  //TODO: offscreen support
  const auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  return osr_rwhv ? osr_rwhv->GetFrameRate() : 0;
#endif
  return 0;
}

void WebContentsBinding::Invalidate() {
  if (!IsOffscreen())
    return;
#if 0
  //TODO: offscreen support
  auto* osr_rwhv = static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
  if (osr_rwhv)
    osr_rwhv->Invalidate();
#endif
}

content::WebContents* WebContentsBinding::HostWebContents() {
  if (!embedder_)
    return nullptr;
  return embedder_->web_contents();
}

void WebContentsBinding::WebViewEmit(const std::string& type, const base::DictionaryValue& params) {
  if (embedder_) {
    auto frame = embedder_->web_contents()->GetMainFrame();
    //auto frame = web_contents()->GetMainFrame();
    LOG(INFO) << __PRETTY_FUNCTION__ << "(" << type << ",?)" << frame->GetRoutingID() << " : " << guest_instance_id_;
    frame->Send(new MesonFrameMsg_WebViewEmit(frame->GetRoutingID(), guest_instance_id_, type, params));
  }
}

void WebContentsBinding::SetEmbedder(void) {
  if (embedder_) {
    NativeWindow* owner_window = nullptr;
    auto relay = NativeWindowRelay::FromWebContents(embedder_->web_contents());
    if (relay) {
      owner_window = relay->window.get();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);

    content::RenderWidgetHostView* rwhv = web_contents()->GetRenderWidgetHostView();
    if (rwhv) {
      rwhv->Hide();
      rwhv->Show();
    }
  }
}
#if 0
  //TODO:
v8::Local<v8::Value> WebContentsBinding::DevToolsWebContents(v8::Isolate* isolate) {
  if (devtools_web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, devtools_web_contents_);
}

v8::Local<v8::Value> WebContentsBinding::Debugger(v8::Isolate* isolate) {
  if (debugger_.IsEmpty()) {
    auto handle = atom::api::Debugger::Create(isolate, web_contents());
    debugger_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, debugger_);
}

// static
void WebContentsBinding::BuildPrototype(v8::Isolate* isolate,
                                             v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "WebContents"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
    .SetMethod("getId", &WebContentsBinding::GetWebContentsID)
      .SetMethod("getProcessId", &WebContentsBinding::GetProcessID)
      .SetMethod("equal", &WebContentsBinding::Equal)
      .SetMethod("_loadURL", &WebContentsBinding::LoadURL)
      .SetMethod("downloadURL", &WebContentsBinding::DownloadURL)
      .SetMethod("_getURL", &WebContentsBinding::GetURL)
      .SetMethod("getTitle", &WebContentsBinding::GetTitle)
      .SetMethod("isLoading", &WebContentsBinding::IsLoading)
      .SetMethod("isLoadingMainFrame", &WebContentsBinding::IsLoadingMainFrame)
      .SetMethod("isWaitingForResponse", &WebContentsBinding::IsWaitingForResponse)
      .SetMethod("_stop", &WebContentsBinding::Stop)
      .SetMethod("_goBack", &WebContentsBinding::GoBack)
      .SetMethod("_goForward", &WebContentsBinding::GoForward)
      .SetMethod("_goToOffset", &WebContentsBinding::GoToOffset)
      .SetMethod("isCrashed", &WebContentsBinding::IsCrashed)
      .SetMethod("setUserAgent", &WebContentsBinding::SetUserAgent)
      .SetMethod("getUserAgent", &WebContentsBinding::GetUserAgent)
      .SetMethod("insertCSS", &WebContentsBinding::InsertCSS)
      .SetMethod("savePage", &WebContentsBinding::SavePage)
      .SetMethod("openDevTools", &WebContentsBinding::OpenDevTools)
      .SetMethod("closeDevTools", &WebContentsBinding::CloseDevTools)
      .SetMethod("isDevToolsOpened", &WebContentsBinding::IsDevToolsOpened)
      .SetMethod("isDevToolsFocused", &WebContentsBinding::IsDevToolsFocused)
      .SetMethod("enableDeviceEmulation", &WebContentsBinding::EnableDeviceEmulation)
      .SetMethod("disableDeviceEmulation", &WebContentsBinding::DisableDeviceEmulation)
      .SetMethod("toggleDevTools", &WebContentsBinding::ToggleDevTools)
      .SetMethod("inspectElement", &WebContentsBinding::InspectElement)
      .SetMethod("setAudioMuted", &WebContentsBinding::SetAudioMuted)
      .SetMethod("isAudioMuted", &WebContentsBinding::IsAudioMuted)
      .SetMethod("undo", &WebContentsBinding::Undo)
      .SetMethod("redo", &WebContentsBinding::Redo)
      .SetMethod("cut", &WebContentsBinding::Cut)
      .SetMethod("copy", &WebContentsBinding::Copy)
      .SetMethod("paste", &WebContentsBinding::Paste)
      .SetMethod("pasteAndMatchStyle", &WebContentsBinding::PasteAndMatchStyle)
      .SetMethod("delete", &WebContentsBinding::Delete)
      .SetMethod("selectAll", &WebContentsBinding::SelectAll)
      .SetMethod("unselect", &WebContentsBinding::Unselect)
      .SetMethod("replace", &WebContentsBinding::Replace)
      .SetMethod("replaceMisspelling", &WebContentsBinding::ReplaceMisspelling)
      .SetMethod("findInPage", &WebContentsBinding::FindInPage)
      .SetMethod("stopFindInPage", &WebContentsBinding::StopFindInPage)
      .SetMethod("focus", &WebContentsBinding::Focus)
      .SetMethod("isFocused", &WebContentsBinding::IsFocused)
      .SetMethod("tabTraverse", &WebContentsBinding::TabTraverse)
      .SetMethod("_send", &WebContentsBinding::SendIPCMessage)
      .SetMethod("sendInputEvent", &WebContentsBinding::SendInputEvent)
      .SetMethod("beginFrameSubscription", &WebContentsBinding::BeginFrameSubscription)
      .SetMethod("endFrameSubscription", &WebContentsBinding::EndFrameSubscription)
      .SetMethod("startDrag", &WebContentsBinding::StartDrag)
      .SetMethod("setSize", &WebContentsBinding::SetSize)
      .SetMethod("isGuest", &WebContentsBinding::IsGuest)
      .SetMethod("isOffscreen", &WebContentsBinding::IsOffScreen)
      .SetMethod("startPainting", &WebContentsBinding::StartPainting)
      .SetMethod("stopPainting", &WebContentsBinding::StopPainting)
      .SetMethod("isPainting", &WebContentsBinding::IsPainting)
      .SetMethod("setFrameRate", &WebContentsBinding::SetFrameRate)
      .SetMethod("getFrameRate", &WebContentsBinding::GetFrameRate)
      .SetMethod("invalidate", &WebContentsBinding::Invalidate)
      .SetMethod("getType", &WebContentsBinding::GetType)
      .SetMethod("getWebPreferences", &WebContentsBinding::GetWebPreferences)
      .SetMethod("getOwnerBrowserWindow", &WebContentsBinding::GetOwnerBrowserWindow)
      .SetMethod("hasServiceWorker", &WebContentsBinding::HasServiceWorker)
      .SetMethod("unregisterServiceWorker",
                 &WebContentsBinding::UnregisterServiceWorker)
      .SetMethod("inspectServiceWorker", &WebContentsBinding::InspectServiceWorker)
      .SetMethod("print", &WebContentsBinding::Print)
      .SetMethod("_printToPDF", &WebContentsBinding::PrintToPDF)
      .SetMethod("addWorkSpace", &WebContentsBinding::AddWorkSpace)
      .SetMethod("removeWorkSpace", &WebContentsBinding::RemoveWorkSpace)
      .SetMethod("showDefinitionForSelection",
                 &WebContentsBinding::ShowDefinitionForSelection)
      .SetMethod("copyImageAt", &WebContentsBinding::CopyImageAt)
      .SetMethod("capturePage", &WebContentsBinding::CapturePage)
      .SetMethod("setEmbedder", &WebContentsBinding::SetEmbedder)
      .SetProperty("id", &WebContentsBinding::ID)
      .SetProperty("session", &WebContentsBinding::Session)
      .SetProperty("hostWebContents", &WebContentsBinding::HostWebContents)
      .SetProperty("devToolsWebContents", &WebContentsBinding::DevToolsWebContents)
      .SetProperty("debugger", &WebContentsBinding::Debugger);
}
#endif

void WebContentsBinding::OnRendererMessage(const base::string16& channel, const base::ListValue& args) {
  // webContents.emit(channel, new Event(), args...);
  //TODO:
  // senderが自分自身
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << channel << ", " << args << ")";
  EmitEvent(base::UTF16ToUTF8(channel) /*, args*/);
}

void WebContentsBinding::OnRendererMessageSync(const base::string16& channel, const base::ListValue& args, IPC::Message* message) {
  // webContents.emit(channel, new Event(sender, message), args...);
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << channel << ", " << args << ")";
  EmitEvent(base::UTF16ToUTF8(channel), "message", /*web_contents(), */ message /*, args*/);
}

WebContentsClassBinding::WebContentsClassBinding(void)
    : APIClassBindingT(MESON_OBJECT_TYPE_WEB_CONTENTS) {}
WebContentsClassBinding::~WebContentsClassBinding() {
}

api::MethodResult WebContentsClassBinding::CreateInstance(const api::APIArgs& args) {
  const base::DictionaryValue* opt = nullptr;
  if (!args.GetDictionary(0, &opt)) {
    return api::MethodResult("invalid argument");
  }
  scoped_refptr<WebContentsBinding> ret(NewInstance(*opt));
  return api::MethodResult(ret);
}

scoped_refptr<WebContentsBinding> WebContentsClassBinding::NewInstance(const base::DictionaryValue& args) {
  auto id = GetNextBindingID();
  scoped_refptr<WebContentsBinding> binding = new WebContentsBinding(id, args);
  SetBinding(id, binding);
  return binding;
}
}
