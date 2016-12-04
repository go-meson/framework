//-*-objc-mode-*-
#pragma once

#include <memory>

#import <Foundation/Foundation.h>

namespace base {
class ListValue;
class DictionaryValue;
}

namespace meson {
NSDictionary* DictionaryValueToNSDictionary(const base::DictionaryValue& value);
std::unique_ptr<base::DictionaryValue> NSDictionaryToDictionaryValue(NSDictionary* dict);
std::unique_ptr<base::ListValue> NSArrayToListValue(NSArray* arr);
}
