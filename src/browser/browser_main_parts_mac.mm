//-*-objc-*-
#include "browser/browser_main_parts.h"

#include "browser/mac/meson_application.h"
#include "browser/mac/meson_application_delegate.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace meson {

void MesonMainParts::PreMainMessageLoopStart() {
  // Force the NSApplication subclass to be used.
  [MesonApplication sharedApplication];

  // Set our own application delegate.
  MesonApplicationDelegate* delegate = [[MesonApplicationDelegate alloc] init];
  [NSApp setDelegate:delegate];

  brightray::BrowserMainParts::PreMainMessageLoopStart();

  // Prevent Cocoa from turning command-line arguments into
  // |-application:openFiles:|, since we already handle them directly.
  [[NSUserDefaults standardUserDefaults]
      setObject:@"NO" forKey:@"NSTreatUnknownArgumentsAsOpen"];
}

void MesonMainParts::FreeAppDelegate() {
  [[NSApp delegate] release];
  [NSApp setDelegate:nil];
}

}
