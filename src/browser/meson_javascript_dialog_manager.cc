#include "browser/meson_javascript_dialog_manager.h"

#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "browser/ui/message_box.h"
#include "browser/native_window.h"

namespace meson {
void MesonJavaScriptDialogManager::RunJavaScriptDialog(content::WebContents* web_contents,
                                                       const GURL& origin_url,
                                                       content::JavaScriptMessageType message_type,
                                                       const base::string16& message_text,
                                                       const base::string16& default_prompt_text,
                                                       const DialogClosedCallback& callback,
                                                       bool* did_suppress_message) {
  if (message_type != content::JavaScriptMessageType::JAVASCRIPT_MESSAGE_TYPE_ALERT &&
      message_type != content::JavaScriptMessageType::JAVASCRIPT_MESSAGE_TYPE_CONFIRM) {
    callback.Run(false, base::string16());
  }

  std::vector<std::string> buttons = {"OK"};
  if (message_type == content::JavaScriptMessageType::JAVASCRIPT_MESSAGE_TYPE_CONFIRM) {
    buttons.push_back("Cancel");
  }

  meson::ShowMessageBox(NativeWindow::FromWebContents(web_contents),
                        meson::MessageBoxType::MESSAGE_BOX_TYPE_NONE,
                        buttons,
                        -1,
                        0,
                        meson::MessageBoxOptions::MESSAGE_BOX_NONE,
                        "",
                        base::UTF16ToUTF8(message_text),
                        "",
                        gfx::ImageSkia(),
                        base::Bind(&OnMessageBoxCallback, callback));
}

void MesonJavaScriptDialogManager::RunBeforeUnloadDialog(content::WebContents* web_contents,
                                                         bool is_reload,
                                                         const DialogClosedCallback& callback) {
  // FIXME(zcbenz): the |message_text| is removed, figure out what should we do.
  callback.Run(false, base::ASCIIToUTF16("This should not be displayed"));
}

void MesonJavaScriptDialogManager::CancelDialogs(content::WebContents* web_contents, bool suppress_callbacks, bool reset_state) {
}

// static
void MesonJavaScriptDialogManager::OnMessageBoxCallback(const DialogClosedCallback& callback, int code) {
  callback.Run(code == 0, base::string16());
}
}
