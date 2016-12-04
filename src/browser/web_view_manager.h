//-*-c++-*-
#pragma once
#include <map>

#include "content/public/browser/browser_plugin_guest_manager.h"

namespace meson {

class WebViewManager : public content::BrowserPluginGuestManager {
 public:
  WebViewManager();
  ~WebViewManager() override;

  void AddGuest(int guest_instance_id,
                int element_instance_id,
                content::WebContents* embedder,
                content::WebContents* web_contents);
  void RemoveGuest(int guest_instance_id);
  content::WebContents* GetEmbedder(int guest_instance_id);

  static WebViewManager* GetWebViewManager(content::WebContents* web_contents);

 protected:
  // content::BrowserPluginGuestManager:
  content::WebContents* GetGuestByInstanceID(int owner_process_id, int element_instance_id) override;
  bool ForEachGuest(content::WebContents* embedder, const GuestCallback& callback) override;

 private:
  struct WebContentsWithEmbedder {
    content::WebContents* web_contents;
    content::WebContents* embedder;
  };
  // guest_instance_id => (web_contents, embedder)
  std::map<int, WebContentsWithEmbedder> web_contents_embedder_map_;

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

