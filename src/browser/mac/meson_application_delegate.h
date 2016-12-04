//-*-objc-*-
#import <Cocoa/Cocoa.h>

#import "browser/ui/cocoa/meson_menu_controller.h"

@interface MesonApplicationDelegate : NSObject<NSApplicationDelegate> {
 @private
  base::scoped_nsobject<MesonMenuController> menu_controller_;
}

// Sets the menu that will be returned in "applicationDockMenu:".
- (void)setApplicationDockMenu:(meson::MesonMenuModel*)model;

@end
