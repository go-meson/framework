//-*-objc-+-
#pragma once

#include "api/menu_binding.h"
#import "browser/ui/cocoa/meson_menu_controller.h"

namespace meson {
  class MesonMenuBindingMac : public MesonMenuBinding {
  public:
    MesonMenuBindingMac(unsigned int id, const api::APICreateArg& args);

    void PopupAt(MesonWindowBinding* window, int x, int y, int positioning_item) override;

    base::scoped_nsobject<MesonMenuController> menu_controller_;
  private:
    static void SendActionToFirstResponder(const std::string& action);

    DISALLOW_COPY_AND_ASSIGN(MesonMenuBindingMac);
  };
}

