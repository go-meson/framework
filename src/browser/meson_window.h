#pragma once

#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
//#include "brightray/browser/defaut_web_contents_delegate.h"
#include "brightray/browser/inspectable_web_contents_delegate.h"
#include "brightray/browser/inspectable_web_contents_impl.h"

#if defined(USE_AURA)
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"
#endif

#if defined(OS_MACOSX)
struct CGContext;
#endif

namespace meson {
class MesonWindow : public content::WebContentsDelegate,
                    public brightray::InspectableWebContentsDelegate,
                    public content::WebContentsObserver,
#if defined(USE_AURA)
                    public views::WidgetDelegateView,
                    public views::WidgetObserver,
#elif defined(OS_MACOSX)
#endif
                    public content::NotificationObserver {
 public:
      static std::unique_ptr<MesonWindow> CreateNew(

      );
private:
};
}