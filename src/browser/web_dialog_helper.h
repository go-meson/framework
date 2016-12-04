//-*-c++-+-
#pragma once
#include "base/memory/weak_ptr.h"

namespace base {
class FilePath;
}

namespace content {
struct FileChooserParams;
class RenderFrameHost;
class WebContents;
}

namespace meson {

class NativeWindow;

class WebDialogHelper {
 public:
  explicit WebDialogHelper(NativeWindow* window);
  ~WebDialogHelper();

  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      const content::FileChooserParams& params);
  void EnumerateDirectory(content::WebContents* web_contents,
                          int request_id,
                          const base::FilePath& path);

 private:
  NativeWindow* window_;

  base::WeakPtrFactory<WebDialogHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebDialogHelper);
};
}
