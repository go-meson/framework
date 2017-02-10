//-*-c++-*-
#pragma once

#include <string>

#include "content/public/browser/javascript_dialog_manager.h"

namespace meson {

class MesonJavaScriptDialogManager : public content::JavaScriptDialogManager {
 public:
  // content::JavaScriptDialogManager implementations.
  void RunJavaScriptDialog(content::WebContents* web_contents,
                           const GURL& origin_url,
                           content::JavaScriptMessageType javascript_message_type,
                           const base::string16& message_text,
                           const base::string16& default_prompt_text,
                           const DialogClosedCallback& callback,
                           bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(content::WebContents* web_contents,
                             bool is_reload,
                             const DialogClosedCallback& callback) override;
  void CancelDialogs(content::WebContents* web_contents, bool suppress_callbacks, bool reset_state) override;
private:
  static void OnMessageBoxCallback(const DialogClosedCallback& callback, int code);
};
}
