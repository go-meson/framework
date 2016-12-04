// -*-c++-*-
#pragma once

#include <memory>
#include "base/callback.h"
#include "base/values.h"

namespace meson {
namespace api {
typedef base::ListValue APIArgs;
typedef base::DictionaryValue APICreateArg;
typedef base::DictionaryValue APIEventArgs;
typedef base::Callback<void(const std::string& error, std::unique_ptr<base::Value> result)> MethodCallback;
typedef base::Callback<void(std::unique_ptr<APIEventArgs> event)> EventCallback;
}
}
