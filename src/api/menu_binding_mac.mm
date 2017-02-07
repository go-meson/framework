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
MenuBindingMac::MenuBindingMac(api::ObjID id)
    : MenuBinding(id) {
}

void MenuBindingMac::PopupAt(WindowBinding* window, int x, int y, int positioning_item) {
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

api::MethodResult MenuClassBinding::SetApplicationMenu(const api::APIArgs& args) {
  const base::DictionaryValue* arg1;
  int type;
  int id;
  if (!args.GetDictionary(0, &arg1) ||
      !arg1->GetInteger("type", &type) ||
      !arg1->GetInteger("id", &id) ||
      (MESON_OBJECT_TYPE_MENU != type)) {
    return api::MethodResult("invalid argument");
  }
  auto base_menu = GetBinding(static_cast<api::ObjID>(id));
  if (!base_menu) {
    return api::MethodResult("invalid menu id");
  }

  MenuBindingMac* menu = static_cast<MenuBindingMac*>(base_menu.get());
  base::scoped_nsobject<MesonMenuController> menu_controller([[MesonMenuController alloc] initWithModel:menu->model_.get()
                                                                                  useDefaultAccelerator:YES]);
  [NSApp setMainMenu:[menu_controller menu]];

  // Ensure the menu_controller_ is destroyed after main menu is set.
  menu_controller.swap(menu->menu_controller_);

  return api::MethodResult();
}

// static
void MenuBinding::SendActionToFirstResponder(const std::string& action) {
  SEL selector = NSSelectorFromString(base::SysUTF8ToNSString(action));
  [NSApp sendAction:selector to:nil from:[NSApp mainMenu]];
}

#if 0
// static
mate::WrappableBase* Menu::New(mate::Arguments* args) {
  return new MenuMac(args->isolate(), args->GetThis());
}
#endif

api::MethodResult MenuClassBinding::CreateInstance(const api::APIArgs& args) {
  auto id = GetNextBindingID();

  scoped_refptr<MenuBinding> binding = new MenuBindingMac(id);
  SetBinding(id, binding);
  LOG(INFO) << "MenuClassBinding::CreateInstance() : id:" << id;

  return api::MethodResult(binding);
}
}
