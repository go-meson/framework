//-*-c++-*-
#pragma once
#include <map>

#include "base/values.h"
#include "content/public/browser/browser_plugin_guest_manager.h"
#include "content/public/browser/render_frame_host.h"

namespace meson {
class MesonWebContentsBinding;
class WebViewManager : public content::BrowserPluginGuestManager {
  class GuestInstance;
  class EmbeddedClient;

 public:
  explicit WebViewManager(void);
  ~WebViewManager() override;

  int GetNextGuestInstanceId() { return ++nextGuestInstanceId_; }
  int GetGuestInstanceID(const content::WebContents* embedder, int element_instance_id);

  void AddGuest(int guest_instance_id,
                int element_instance_id,
                content::WebContents* embedder,
                content::WebContents* web_contents);
  void RemoveGuest(int guest_instance_id);
  content::WebContents* GetEmbedder(int guest_instance_id);

  static WebViewManager* GetWebViewManager(content::WebContents* web_contents);

#if 0
 public:
  bool OnMessageReceived(const IPC::Message& message, content::WebContents* web_content);
#endif

 public:
  void OnCreateWebViewGuest(content::WebContents* web_contents, const base::DictionaryValue& params, int* guest_instance_id);
  void OnAttachWindowGuest(content::WebContents* web_contents, int internal_instance_id, int guest_instance_id, const base::DictionaryValue& params);
  void OnDestroyWebViewGuest(int guest_instance_id);
  void OnWebViewGuestSetAutoSize(content::WebContents* web_contents, int guest_instance_id, const base::DictionaryValue& params);
  void OnWebViewGuestGo(content::WebContents* web_contents, int guest_instance_id, int relative_index);
  void OnWebViewGuestLoadUrl(content::WebContents* web_contents, int guest_instance_id, const std::string& url);
  void OnWebViewGuestReload(content::WebContents* web_contents, int guest_instance_id, bool ignore_cache);
  void OnWebViewGuestStop(content::WebContents* web_contents, int guest_instance_id);
  void OnWebViewGuestSetZoom(content::WebContents* web_contents, int guest_instance_id, double zoom_factor);
  void OnWebViewGuestFind(content::WebContents* web_contents, int guest_instance_id, int request_id, const std::string& search_text, const base::DictionaryValue& options);
  void OnWebViewGuestStopFinding(content::WebContents* web_contents, int guest_instance_id, const std::string& action);
  void OnWebViewGuestInsertCSS(content::WebContents* web_contents, int guest_instance_id, const std::string& css);
  void OnWebViewGuestExecuteScript(content::WebContents* web_contents, int guest_instance_id, const std::string& script);
  void OnWebViewGuestOpenDevTools(content::WebContents* web_contents, int guest_instance_id);
  void OnWebViewGuestCloseDevTools(content::WebContents* web_contents, int guest_instance_id);
  void OnWebViewGuestIsDevToolsOpened(int guest_instance_id, bool* open);
  void OnWebViewGuestJavaScriptDialogClosed(content::WebContents* web_contents, int guest_instance_id, bool success, const std::string& response);

 public:
  void WatchEmbedder(scoped_refptr<MesonWebContentsBinding> embedder);
  void OnDestroyEmbedder(MesonWebContentsBinding* embedder);

 protected:
  // content::BrowserPluginGuestManager:
  content::WebContents* GetGuestByInstanceID(int owner_process_id, int element_instance_id) override;
  bool ForEachGuest(content::WebContents* embedder, const GuestCallback& callback) override;

 private:
  int nextGuestInstanceId_;
  struct WebContentsWithEmbedder {
    content::WebContents* web_contents;
    content::WebContents* embedder;
  };
  // guest_instance_id => (web_contents, embedder)
  std::map<int, WebContentsWithEmbedder> web_contents_embedder_map_;
  // guestInstanceID -> GuestInstance
  std::map<int, scoped_refptr<GuestInstance>> guest_instances_;
  // embedder's WebContentsBindingID -> EmbeddedClient
  std::map<int, scoped_refptr<EmbeddedClient>> embedded_clients_;

  struct ElementInstanceKey {
    int embedder_process_id;
    int element_instance_id;

    ElementInstanceKey(int embedder_process_id, int element_instance_id)
        : embedder_process_id(embedder_process_id),
          element_instance_id(element_instance_id) {}

    bool operator<(const ElementInstanceKey& other) const {
      if (embedder_process_id != other.embedder_process_id)
        return embedder_process_id < other.embedder_process_id;
      return element_instance_id < other.element_instance_id;
    }

    bool operator==(const ElementInstanceKey& other) const {
      return (embedder_process_id == other.embedder_process_id) &&
             (element_instance_id == other.element_instance_id);
    }
  };
  // (embedder_process_id, element_instance_id) => guest_instance_id
  std::map<ElementInstanceKey, int> element_instance_id_to_guest_map_;

  DISALLOW_COPY_AND_ASSIGN(WebViewManager);
};
}
