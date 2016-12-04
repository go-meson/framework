//-*-objc-*-
#pragma once
#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/string16.h"

namespace meson {
class MesonMenuModel;
}

// A controller for the cross-platform menu model. The menu that's created
// has the tag and represented object set for each menu item. The object is a
// NSValue holding a pointer to the model for that level of the menu (to
// allow for hierarchical menus). The tag is the index into that model for
// that particular item. It is important that the model outlives this object
// as it only maintains weak references.
@interface MesonMenuController : NSObject<NSMenuDelegate> {
 @protected
  meson::MesonMenuModel* model_;  // weak
  base::scoped_nsobject<NSMenu> menu_;
  BOOL isMenuOpen_;
  BOOL useDefaultAccelerator_;
}

@property(nonatomic, assign) meson::MesonMenuModel* model;

// Builds a NSMenu from the pre-built model (must not be nil). Changes made
// to the contents of the model after calling this will not be noticed.
- (id)initWithModel:(meson::MesonMenuModel*)model useDefaultAccelerator:(BOOL)use;

// Populate current NSMenu with |model|.
- (void)populateWithModel:(meson::MesonMenuModel*)model;

// Programmatically close the constructed menu.
- (void)cancel;

// Access to the constructed menu if the complex initializer was used. If the
// default initializer was used, then this will create the menu on first call.
- (NSMenu*)menu;

// Whether the menu is currently open.
- (BOOL)isMenuOpen;

// NSMenuDelegate methods this class implements. Subclasses should call super
// if extending the behavior.
- (void)menuWillOpen:(NSMenu*)menu;
- (void)menuDidClose:(NSMenu*)menu;

@end
