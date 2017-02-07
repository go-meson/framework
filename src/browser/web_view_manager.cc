#include "browser/web_view_manager.h"

#include "base/strings/utf_string_conversions.h"
#include "browser/session/meson_browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/WebKit/public/web/WebFindOptions.h"
#include "api/web_contents_binding.h"
#include "api/api_messages.h"
#include "common/options_switches.h"
#include "api/api.h"
#include "api/api_binding.h"
#include "browser/web_view_guest_delegate.h"
#include "browser/web_contents_preferences.h"

namespace meson {
class IWebViewClient {
 public:
  virtual void EmitEvent(scoped_refptr<api::EventArg> event) = 0;
};

class WebViewBindingRemote : public APIBindingRemote {
  IWebViewClient& client_;

 public:
  WebViewBindingRemote(IWebViewClient& client, scoped_refptr<WebContentsBinding> c)
      : APIBindingRemote(c), client_(client) {
    LOG(INFO) << __PRETTY_FUNCTION__ << " : " << Binding()->GetID();
  }
  void RemoveBinding(APIBinding* binding) override {
    LOG(INFO) << __PRETTY_FUNCTION__;
    CHECK(binding_.get() == binding);
    binding_ = nullptr;
  }

  bool InvokeMethod(const std::string method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback) override {
    //TODO: need implement?
    return false;
  }
  void EmitEvent(scoped_refptr<api::EventArg> event) override {
    if (!binding_) {
      return;
    }
    client_.EmitEvent(event);
  }
  std::unique_ptr<base::Value> EmitEventWithResult(scoped_refptr<api::EventArg> event) override {
    //TODO:
    return std::unique_ptr<base::Value>();
  }
};

class WebViewManager::EmbeddedClient final : public base::RefCountedThreadSafe<EmbeddedClient>, public IWebViewClient {
  WebViewManager& mgr_;
  unsigned int binding_id_;
  scoped_refptr<WebViewBindingRemote> remote_;

public:
  EmbeddedClient(WebViewManager& m, scoped_refptr<WebContentsBinding> c)
      : mgr_(m),
        binding_id_(c->GetID()),
        remote_(make_scoped_refptr(new WebViewBindingRemote(*this, c))) {
    LOG(INFO) << __PRETTY_FUNCTION__ << " : " << binding_id_;
    WebContentsBinding::Class().SetRemote(c->GetID(), remote_.get());
  }

  ~EmbeddedClient() {
    LOG(INFO) << __PRETTY_FUNCTION__ << " (" << (long )this;
    //LOG(INFO) << __PRETTY_FUNCTION__ << " : " << binding_->GetID();
    WebContentsBinding::Class().RemoveRemote(binding_id_, remote_.get());
  }
  void EmitEvent(scoped_refptr<api::EventArg> event) override {
    LOG(INFO) << __PRETTY_FUNCTION__ << " : " << event->name_;
    static const char* destroyEvents[] = {"will-destroy", "crashed", "did-navigate"};
    if (std::find(std::begin(destroyEvents), std::end(destroyEvents), event->name_) != std::end(destroyEvents)) {
      auto binding = WebContentsBinding::Class().GetBinding(binding_id_);
      if (binding) {
        mgr_.OnDestroyEmbedder(binding.get());
      }
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(EmbeddedClient);
};

class WebViewManager::GuestInstance final : public base::RefCountedThreadSafe<GuestInstance>, public IWebViewClient {
 public:
  WebViewManager& mgr_;
  unsigned int binding_id_;
  int guestInstanceId;
  int elementInstanceId;
  base::DictionaryValue attachParams;
  scoped_refptr<WebViewBindingRemote> remote_;

 private:
  bool allowPopups;

 public:
  GuestInstance(WebViewManager& m, int giid, scoped_refptr<WebContentsBinding> c)
      : mgr_(m),
        binding_id_(c->GetID()),
        guestInstanceId(giid),
        elementInstanceId(-1),
        remote_(make_scoped_refptr(new WebViewBindingRemote(*this, c))),
        allowPopups(false) {
    LOG(INFO) << __PRETTY_FUNCTION__ << " : " << binding_id_;
    WebContentsBinding::Class().SetRemote(c->GetID(), remote_.get());
  }
  ~GuestInstance() {
    LOG(INFO) << __PRETTY_FUNCTION__ << " : " << binding_id_;
    WebContentsBinding::Class().RemoveRemote(binding_id_, remote_.get());
  }

 public:
  scoped_refptr<WebContentsBinding> Binding() const { return WebContentsBinding::Class().GetBinding(binding_id_); }

 public:
  void EmitEvent(scoped_refptr<api::EventArg> event) override {
    auto& name = event->name_;
    LOG(INFO) << __PRETTY_FUNCTION__ << "[" << guestInstanceId << "] : " << name;
    if (name == "did-attach") {
      OnDidAttach();
    }
    Binding()->WebViewEmit(name, *event->event_);
  }

 private:
  void OnDidAttach() {
    auto binding = Binding();
    if (!binding) {
      return;
    }
    base::DictionaryValue params;
    params.Swap(&attachParams);

    // setsize
    SetSizeParams sp;
    int w = 0;
    int h = 0;
    if (params.GetInteger("elementWidth", &w) && params.GetInteger("elementHeight", &h)) {
      sp.normal_size.reset(new gfx::Size(w, h));
    }
    bool b;
    if (attachParams.GetBoolean("autosize", &b) && b) {
      sp.enable_auto_size.reset(new bool(b));
    }
    if (params.GetInteger("minwidth", &w) && params.GetInteger("minheight", &h)) {
      sp.min_size.reset(new gfx::Size(w, h));
    }
    if (params.GetInteger("maxwidth", &w) && params.GetInteger("maxheight", &h)) {
      sp.max_size.reset(new gfx::Size(w, h));
    }
    binding->GetGuestDelegate()->SetSize(sp);

    // load
    std::string url;
    if (params.GetString("src", &url)) {
      base::DictionaryValue opt;
      //TODO: opts
      binding->LoadURL(GURL(url), opt);
    }
    params.GetBoolean("allowpopups", &allowPopups);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(GuestInstance);
};

WebViewManager::WebViewManager(void)
    : nextGuestInstanceId_(0) {
  LOG(INFO) << __PRETTY_FUNCTION__;
}

WebViewManager::~WebViewManager() {
  LOG(INFO) << __PRETTY_FUNCTION__;
}

int WebViewManager::GetGuestInstanceID(const content::WebContents* embedder, int element_instance_id) {
  int owner_process_id = embedder->GetRenderProcessHost()->GetID();
  ElementInstanceKey key(owner_process_id, element_instance_id);
  auto fiter = element_instance_id_to_guest_map_.find(key);
  if (fiter == element_instance_id_to_guest_map_.end()) {
    return -1;
  }
  return (*fiter).second;
}

void WebViewManager::AddGuest(int guest_instance_id,
                              int element_instance_id,
                              content::WebContents* embedder,
                              content::WebContents* web_contents) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  web_contents_embedder_map_[guest_instance_id] = {web_contents, embedder};

  // Map the element in embedder to guest.
  int owner_process_id = embedder->GetRenderProcessHost()->GetID();
  ElementInstanceKey key(owner_process_id, element_instance_id);
  element_instance_id_to_guest_map_[key] = guest_instance_id;
}

void WebViewManager::RemoveGuest(int guest_instance_id) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (!ContainsKey(web_contents_embedder_map_, guest_instance_id))
    return;

  web_contents_embedder_map_.erase(guest_instance_id);

  // Remove the record of element in embedder too.
  for (const auto& element : element_instance_id_to_guest_map_)
    if (element.second == guest_instance_id) {
      element_instance_id_to_guest_map_.erase(element.first);
      break;
    }
}

content::WebContents* WebViewManager::GetEmbedder(int guest_instance_id) {
  if (ContainsKey(web_contents_embedder_map_, guest_instance_id))
    return web_contents_embedder_map_[guest_instance_id].embedder;
  else
    return nullptr;
}

content::WebContents* WebViewManager::GetGuestByInstanceID(
    int owner_process_id,
    int element_instance_id) {
  ElementInstanceKey key(owner_process_id, element_instance_id);
  if (!ContainsKey(element_instance_id_to_guest_map_, key))
    return nullptr;

  int guest_instance_id = element_instance_id_to_guest_map_[key];
  if (ContainsKey(web_contents_embedder_map_, guest_instance_id))
    return web_contents_embedder_map_[guest_instance_id].web_contents;
  else
    return nullptr;
}

bool WebViewManager::ForEachGuest(content::WebContents* embedder_web_contents,
                                  const GuestCallback& callback) {
  for (auto& item : web_contents_embedder_map_)
    if (item.second.embedder == embedder_web_contents && callback.Run(item.second.web_contents))
      return true;
  return false;
}

// static
WebViewManager* WebViewManager::GetWebViewManager(content::WebContents* web_contents) {
  auto context = web_contents->GetBrowserContext();
  if (context) {
    auto manager = context->GetGuestManager();
    auto webViewManager = static_cast<WebViewManager*>(manager);
    return webViewManager;
  } else {
    return nullptr;
  }
}

#if 0
bool WebViewManager::OnMessageReceived(const IPC::Message& message, content::WebContents* web_contents) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(WebViewManager, message, web_contents)
    IPC_MESSAGE_UNHANDLED(hancdled = false)
    IPC_MESSAGE_HANDLER(MesonFrameHostMsg_CreateWebViewGuest, OnCreateWebViewGuest)
  IPC_END_MESSAGE_MAP()
  return handled;
}
#endif

void WebViewManager::OnCreateWebViewGuest(content::WebContents* web_contents, const base::DictionaryValue& params, int* guest_instance_id) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << params << ")";
  int guestInstanceId = GetNextGuestInstanceId();
  std::unique_ptr<base::DictionaryValue> opt(new base::DictionaryValue());
  opt->SetBoolean("isGuest", true);
  opt->SetInteger("guest_instance_id", guestInstanceId);
  std::string partition;
  if (params.GetStringWithoutPathExpansion("partition", &partition)) {
    opt->SetString("partition", partition);
  }

  auto parentapi = WebContentsBinding::Class().FindBinding([web_contents](const WebContentsBinding& b) {
    return b.GetWebContents() == web_contents;
  });
  CHECK(parentapi) << "Parent api is not found!";
  WatchEmbedder(parentapi.get());
  opt->SetInteger("embedder", parentapi->GetID());
  auto api = static_cast<WebContentsClassBinding&>(WebContentsBinding::Class()).NewInstance(*opt);

  auto client = make_scoped_refptr(new GuestInstance(*this, guestInstanceId, api));
  guest_instances_[guestInstanceId] = client;

  *guest_instance_id = guestInstanceId;
}

void WebViewManager::OnAttachWindowGuest(content::WebContents* web_contents, int internal_instance_id, int guest_instance_id, const base::DictionaryValue& params) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << internal_instance_id << ", " << guest_instance_id << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  // If this isn't a valid guest instance then do nothing.
  if (fiter == guest_instances_.end()) {
    LOG(INFO) << "guest(" << guest_instance_id << ") is already exists!";
    return;
  }
  auto client = (*fiter).second;
  CHECK(client);

  auto guest = client->Binding();
  CHECK(guest);
  WebViewManager* webViewManager = WebViewManager::GetWebViewManager(guest->GetEmbedder()->web_contents());
  int oldGuestInstanceID = webViewManager->GetGuestInstanceID(web_contents, internal_instance_id);
  if (oldGuestInstanceID >= 0) {
    if (oldGuestInstanceID == guest_instance_id) {
      LOG(INFO) << "Reattachment!";
      // Reattachment to the same guest is just a no-op.
      return;
    }
    LOG(INFO) << "Destroy old guest!";
    OnDestroyWebViewGuest(oldGuestInstanceID);
  }

  // If this guest is already attached to an element then remove it
  if (client->elementInstanceId >= 0) {
    RemoveGuest(guest_instance_id);
    //TODO: need to send destroyguest
    //guestInstance.embedder->Send(IPC::Message *msg)
    LOG(ERROR) << "Not implement yet. need destroy guest";
  }

  // TODO: 何か違う…
  client->attachParams.MergeDictionary(&params);

  base::DictionaryValue webPreferences;

  webPreferences.SetInteger("guestInstanceId", guest_instance_id);
  double d;
  bool b;
  if (params.GetDoubleWithoutPathExpansion("zoomFactor", &d)) {
    webPreferences.SetDouble(options::kZoomFactor, d);
  }
  const base::Value* v;
  if (params.Get("plugins", &v)) {
    webPreferences.Set("plugins", v->DeepCopy());
  }
  if (params.GetBooleanWithoutPathExpansion("disablewebsecurity", &b)) {
    webPreferences.SetBoolean("webSecurity", !b);
  }
  if (params.Get("blinkfeatures", &v)) {
    webPreferences.Set(options::kBlinkFeatures, v->DeepCopy());
  }
  if (params.GetBooleanWithoutPathExpansion("disableblinkfeatures", &b)) {
    webPreferences.SetBoolean(options::kDisableBlinkFeatures, b);
  }
  std::string s;
  if (params.GetStringWithoutPathExpansion("preload", &s)) {
    webPreferences.SetString(options::kPreloadURL, s);
  }
  webViewManager->AddGuest(guest_instance_id, internal_instance_id, web_contents, guest->web_contents());
  WebContentsPreferences::FromWebContents(guest->web_contents())->Merge(webPreferences);
  guest->SetEmbedder();
  client->elementInstanceId = internal_instance_id;
  WatchEmbedder(guest->GetEmbedder());
}

void WebViewManager::OnDestroyWebViewGuest(int guest_instance_id) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  scoped_refptr<GuestInstance> guest;
  if (fiter != guest_instances_.end()) {
    guest = (*fiter).second;
    guest_instances_.erase(fiter);
  }
  if (guest) {
    auto binding = guest->Binding();
    if (binding) {
      binding->GetGuestDelegate()->Destroy();
    }
  }
}
void WebViewManager::OnWebViewGuestSetAutoSize(content::WebContents* web_contents, int guest_instance_id, const base::DictionaryValue& params) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ", " << params << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto client = (*fiter).second;
  auto api = client->Binding();
  CHECK(api);
  auto guest = api->GetGuestDelegate();

  SetSizeParams sp;
  int w = 0;
  int h = 0;
  if (params.GetInteger("normal.width", &w) && params.GetInteger("normal.height", &h)) {
    sp.normal_size.reset(new gfx::Size(w, h));
  }
  if (params.GetInteger("min_size.width", &w) && params.GetInteger("min_size.height", &h)) {
    sp.min_size.reset(new gfx::Size(w, h));
  }
  if (params.GetInteger("max_size.width", &w) && params.GetInteger("max_size.height", &h)) {
    sp.max_size.reset(new gfx::Size(w, h));
  }
  bool enabled = false;
  if (params.GetBoolean("enableAutoSize", &enabled)) {
    LOG(INFO) << "enableAutoSIze ==>" << enabled;
    sp.enable_auto_size.reset(new bool(enabled));
  }

  guest->SetSize(sp);
}

void WebViewManager::OnWebViewGuestLoadUrl(content::WebContents* web_contents, int guest_instance_id, const std::string& url) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ", " << url << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto guest = (*fiter).second;
  auto api = guest->Binding();
  CHECK(api);
  base::DictionaryValue opt;
  api->LoadURL(GURL(url), opt);
}

void WebViewManager::OnWebViewGuestGo(content::WebContents* web_contents, int guest_instance_id, int relative_index) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ", " << relative_index << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto guest = (*fiter).second;
  auto api = guest->Binding();
  CHECK(api);
  api->GoToOffset(relative_index);
}

void WebViewManager::OnWebViewGuestReload(content::WebContents* web_contents, int guest_instance_id, bool ignore_cache) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ", " << ignore_cache << ")"
            << "is not implement yet.";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  //auto& guestInstance = (*fiter).second;
  //auto api = guestInstance.binding;
  //TODO:
  //api->GoToOffset(relative_index);
}

void WebViewManager::OnWebViewGuestStop(content::WebContents* web_contents, int guest_instance_id) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto guest = (*fiter).second;
  auto api = guest->Binding();
  CHECK(api);
  api->Stop();
}

void WebViewManager::OnWebViewGuestSetZoom(content::WebContents* web_contents, int guest_instance_id, double zoom_factor) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ")"
            << "is not implement yet.";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  //auto& guestInstance = (*fiter).second;
  //auto api = guestInstance.binding;
  //TODO:
  //auto api = (*fiter).second;
  //auto& guest = api->guest_delegate_;
}

void WebViewManager::OnWebViewGuestFind(content::WebContents* web_contents, int guest_instance_id, int request_id, const std::string& search_text, const base::DictionaryValue& options) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ", " << search_text << ", " << options << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto guest = (*fiter).second;
  auto api = guest->Binding();
  CHECK(api);
  base::string16 search_text16 = base::UTF8ToUTF16(search_text);
  blink::WebFindOptions findopt;
  options.GetBoolean("forward", &findopt.forward);
  options.GetBoolean("match_case", &findopt.matchCase);
  options.GetBoolean("find_next", &findopt.findNext);
  options.GetBoolean("word_start", &findopt.wordStart);
  options.GetBoolean("medial_capital_as_word_start", &findopt.medialCapitalAsWordStart);

  api->web_contents()->Find(request_id, search_text16, findopt);
}

void WebViewManager::OnWebViewGuestStopFinding(content::WebContents* web_contents, int guest_instance_id, const std::string& action) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ", " << action << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto guest = (*fiter).second;
  auto api = guest->Binding();
  CHECK(api);
  content::StopFindAction action_value = content::STOP_FIND_ACTION_CLEAR_SELECTION;
  if (action.compare("clear") == 0) {
    action_value = content::STOP_FIND_ACTION_CLEAR_SELECTION;
  }
  if (action.compare("keep") == 0) {
    action_value = content::STOP_FIND_ACTION_KEEP_SELECTION;
  }
  if (action.compare("activate") == 0) {
    action_value = content::STOP_FIND_ACTION_ACTIVATE_SELECTION;
  }
  api->StopFindInPage(action_value);
}
void WebViewManager::OnWebViewGuestInsertCSS(content::WebContents* web_contents, int guest_instance_id, const std::string& css) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ", " << css << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto guest = (*fiter).second;
  auto api = guest->Binding();
  CHECK(api);
  api->InsertCSS(css);
}
void WebViewManager::OnWebViewGuestExecuteScript(content::WebContents* web_contents, int guest_instance_id, const std::string& script) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ", " << script << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto guest = (*fiter).second;
  auto api = guest->Binding();
  CHECK(api);
  api->web_contents()->GetMainFrame()->ExecuteJavaScript(base::UTF8ToUTF16(script));
}
void WebViewManager::OnWebViewGuestOpenDevTools(content::WebContents* web_contents, int guest_instance_id) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto guest = (*fiter).second;
  auto api = guest->Binding();
  CHECK(api);
  api->OpenDevTools(nullptr);
}
void WebViewManager::OnWebViewGuestCloseDevTools(content::WebContents* web_contents, int guest_instance_id) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto guest = (*fiter).second;
  auto api = guest->Binding();
  CHECK(api);
  api->CloseDevTools();
}
void WebViewManager::OnWebViewGuestIsDevToolsOpened(int guest_instance_id, bool* open) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ")";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  auto guest = (*fiter).second;
  auto api = guest->Binding();
  CHECK(api);
  *open = api->IsDevToolsOpened();
}
void WebViewManager::OnWebViewGuestJavaScriptDialogClosed(content::WebContents* web_contents, int guest_instance_id, bool success, const std::string& response) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ", " << success << ", " << response << ")"
            << " is not implement yet.";
  auto fiter = guest_instances_.find(guest_instance_id);
  CHECK(fiter != guest_instances_.end()) << " invalid guest id.";
  //auto& guestInstance = (*fiter).second;
  //auto api = guestInstance.binding;
  //TODO:
  //auto api = (*fiter).second;
}

void WebViewManager::WatchEmbedder(scoped_refptr<WebContentsBinding> embedder) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << embedder->GetID();
  unsigned int embedder_id = embedder->GetID();
  auto fiter = embedded_clients_.find(embedder_id);
  if (fiter != embedded_clients_.end()) {
    // already registered
    return;
  }
  auto client = make_scoped_refptr(new EmbeddedClient(*this, embedder));
  embedded_clients_[embedder_id] = client;
}

void WebViewManager::OnDestroyEmbedder(WebContentsBinding* embedder) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << embedder->GetID();
  auto emb = make_scoped_refptr(embedder);
  unsigned int embedder_id = emb->GetID();
  std::vector<int> destroyed_guests;
  for (auto& kv : guest_instances_) {
    auto e = kv.second->Binding()->GetEmbedder();
    CHECK(e);
    if (e->GetID() == embedder_id) {
      destroyed_guests.push_back(kv.first);
    }
  }
  embedded_clients_.erase(emb->GetID());
  for (auto giid : destroyed_guests) {
    OnDestroyWebViewGuest(giid);
  }
}
}
