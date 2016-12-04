//-*-objc-*-

#import "base/mac/scoped_sending_event.h"
#import "base/mac/scoped_nsobject.h"

@interface MesonApplication : NSApplication<CrAppProtocol, CrAppControlProtocol> {
 @private
  BOOL handlingSendEvent_;
  base::scoped_nsobject<NSUserActivity> currentActivity_;
}

+ (MesonApplication*)sharedApplication;

// CrAppProtocol:
- (BOOL)isHandlingSendEvent;

// CrAppControlProtocol:
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;

- (NSUserActivity*)getCurrentActivity;
- (void)setCurrentActivity:(NSString*)type
              withUserInfo:(NSDictionary*)userInfo
            withWebpageURL:(NSURL*)webpageURL;

@end
