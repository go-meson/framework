//-*-objc-*-
#import "browser/mac/meson_application_delegate.h"

#import "browser/mac/meson_application.h"
#include "browser/browser.h"
#include "browser/mac/dict_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"

@interface NSWindow (SierraSDK)
@property(class) BOOL allowsAutomaticWindowTabbing;
@end

@implementation MesonApplicationDelegate

- (void)setApplicationDockMenu:(meson::MesonMenuModel*)model {
  LOG(INFO) << "++++****" << __PRETTY_FUNCTION__;
  menu_controller_.reset([[MesonMenuController alloc] initWithModel:model useDefaultAccelerator:NO]);
}

- (void)applicationWillFinishLaunching:(NSNotification*)notify {
  // Don't add the "Enter Full Screen" menu item automatically.
  [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];

  // Don't add the "Show Tab Bar" menu item.
  if ([NSWindow respondsToSelector:@selector(allowsAutomaticWindowTabbing)])
    NSWindow.allowsAutomaticWindowTabbing = NO;

  meson::Browser::Get()->WillFinishLaunching();
}

- (void)applicationDidFinishLaunching:(NSNotification*)notify {
  NSUserNotification* user_notification = [notify userInfo][(id) @"NSApplicationLaunchUserNotificationKey"];

  if (user_notification.userInfo != nil) {
    std::unique_ptr<base::DictionaryValue> launch_info = meson::NSDictionaryToDictionaryValue(user_notification.userInfo);
    meson::Browser::Get()->DidFinishLaunching(*launch_info);
  } else {
    std::unique_ptr<base::DictionaryValue> empty_info(new base::DictionaryValue);
    meson::Browser::Get()->DidFinishLaunching(*empty_info);
  }
}

- (NSMenu*)applicationDockMenu:(NSApplication*)sender {
  if (menu_controller_)
    return [menu_controller_ menu];
  else
    return nil;
}

- (BOOL)application:(NSApplication*)sender openFile:(NSString*)filename {
  std::string filename_str(base::SysNSStringToUTF8(filename));
  return meson::Browser::Get()->OpenFile(filename_str) ? YES : NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
  meson::Browser* browser = meson::Browser::Get();
  if (browser->is_quiting()) {
    return NSTerminateNow;
  } else {
    // System started termination.
    meson::Browser::Get()->Quit();
    return NSTerminateCancel;
  }
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication
                    hasVisibleWindows:(BOOL)flag {
  meson::Browser* browser = meson::Browser::Get();
  browser->Activate(static_cast<bool>(flag));
  return flag;
}

- (BOOL)application:(NSApplication*)sender
    continueUserActivity:(NSUserActivity*)userActivity
      restorationHandler:(void (^)(NSArray* restorableObjects))restorationHandler {
  std::string activity_type(base::SysNSStringToUTF8(userActivity.activityType));
  std::unique_ptr<base::DictionaryValue> user_info = meson::NSDictionaryToDictionaryValue(userActivity.userInfo);
  if (!user_info)
    return NO;

  meson::Browser* browser = meson::Browser::Get();
  return browser->ContinueUserActivity(activity_type, *user_info) ? YES : NO;
}

@end
