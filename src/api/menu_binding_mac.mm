//-*-objc-*-
// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "api/menu_binding_mac.h"

#include "browser/native_window.h"
#include "browser/unresponsive_suppressor.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "content/public/browser/web_contents.h"

namespace meson {
MesonMenuBindingMac::MesonMenuBindingMac(unsigned int id, const api::APICreateArg& args)
    : MesonMenuBinding(id, args) {
}

void MesonMenuBindingMac::PopupAt(MesonWindowBinding* window, int x, int y, int positioning_item) {
  NativeWindow* native_window = window->window();
  if (!native_window)
    return;
  brightray::InspectableWebContents* web_contents = native_window->inspectable_web_contents();
  if (!web_contents)
    return;

  base::scoped_nsobject<MesonMenuController> menu_controller([[MesonMenuController alloc] initWithModel:model_.get()
                                                                                  useDefaultAccelerator:NO]);
  NSMenu* menu = [menu_controller menu];
  NSView* view = web_contents->GetView()->GetNativeView();

  // Which menu item to show.
  NSMenuItem* item = nil;
  if (positioning_item < [menu numberOfItems] && positioning_item >= 0)
    item = [menu itemAtIndex:positioning_item];

  // (-1, -1) means showing on mouse location.
  NSPoint position;
  if (x == -1 || y == -1) {
    NSWindow* nswindow = native_window->GetNativeWindow();
    position = [view convertPoint:[nswindow mouseLocationOutsideOfEventStream]
                         fromView:nil];
  } else {
    position = NSMakePoint(x, [view frame].size.height - y);
  }

  // If no preferred item is specified, try to show all of the menu items.
  if (!positioning_item) {
    CGFloat windowBottom = CGRectGetMinY([view window].frame);
    CGFloat lowestMenuPoint = windowBottom + position.y - [menu size].height;
    CGFloat screenBottom = CGRectGetMinY([view window].screen.frame);
    CGFloat distanceFromBottom = lowestMenuPoint - screenBottom;
    if (distanceFromBottom < 0)
      position.y = position.y - distanceFromBottom + 4;
  }

  // Place the menu left of cursor if it is overflowing off right of screen.
  CGFloat windowLeft = CGRectGetMinX([view window].frame);
  CGFloat rightmostMenuPoint = windowLeft + position.x + [menu size].width;
  CGFloat screenRight = CGRectGetMaxX([view window].screen.frame);
  if (rightmostMenuPoint > screenRight)
    position.x = position.x - [menu size].width;

  // Don't emit unresponsive event when showing menu.
  meson::UnresponsiveSuppressor suppressor;

  // Show the menu.
  [menu popUpMenuPositioningItem:item atLocation:position inView:view];
}

// static
void MesonMenuBinding::SetApplicationMenu(MesonMenuBinding* base_menu) {
  MesonMenuBindingMac* menu = static_cast<MesonMenuBindingMac*>(base_menu);
  base::scoped_nsobject<MesonMenuController> menu_controller([[MesonMenuController alloc] initWithModel:menu->model_.get()
                                                                                  useDefaultAccelerator:YES]);
  [NSApp setMainMenu:[menu_controller menu]];

  // Ensure the menu_controller_ is destroyed after main menu is set.
  menu_controller.swap(menu->menu_controller_);
}

// static
void MesonMenuBinding::SendActionToFirstResponder(const std::string& action) {
  SEL selector = NSSelectorFromString(base::SysUTF8ToNSString(action));
  [NSApp sendAction:selector to:nil from:[NSApp mainMenu]];
}

#if 0
// static
mate::WrappableBase* Menu::New(mate::Arguments* args) {
  return new MenuMac(args->isolate(), args->GetThis());
}
#endif

APIBinding* MesonMenuFactory::Create(unsigned int id, const api::APICreateArg& args) {
  return new MesonMenuBindingMac(id, args);
}
}
