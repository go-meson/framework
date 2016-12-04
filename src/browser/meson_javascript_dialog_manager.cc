#include "browser/meson_javascript_dialog_manager.h"

#include <string>

#include "base/strings/utf_string_conversions.h"

namespace meson {
void MesonJavaScriptDialogManager::RunJavaScriptDialog(content::WebContents* web_contents,
                                                       const GURL& origin_url,
                                                       content::JavaScriptMessageType javascript_message_type,
                                                       const base::string16& message_text,
                                                       const base::string16& default_prompt_text,
                                                       const DialogClosedCallback& callback,
                                                       bool* did_suppress_message) {
  callback.Run(false, base::string16());
}

void MesonJavaScriptDialogManager::RunBeforeUnloadDialog(content::WebContents* web_contents,
                                                         bool is_reload,
                                                         const DialogClosedCallback& callback) {
  // FIXME(zcbenz): the |message_text| is removed, figure out what should we do.
  callback.Run(false, base::ASCIIToUTF16("This should not be displayed"));
}
}
