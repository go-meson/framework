//-*-c++-*-
#pragma once

#include <vector>
#include "base/values.h"
#include "url/gurl.h"
#include "ui/gfx/geometry/rect.h"

namespace meson {
namespace internal {
inline void ToArgs(base::DictionaryValue& dict) {}
inline void ToArg(base::DictionaryValue& dict, const char* name, int a) {
  dict.SetInteger(name, a);
}
inline void ToArg(base::DictionaryValue& dict, const char* name, double d) {
  dict.SetDouble(name, d);
}
inline void ToArg(base::DictionaryValue& dict, const char* name, bool b) {
  dict.SetBoolean(name, b);
}
inline void ToArg(base::DictionaryValue& dict, const char* name, const std::string& s) {
  dict.SetString(name, s);
}
inline void ToArg(base::DictionaryValue& dict, const char* name, const base::string16& s) {
  dict.SetString(name, s);
}
inline void ToArg(base::DictionaryValue& dict, const char* name, const GURL& url) {
  dict.SetString(name, url.spec());
}
inline void ToArg(base::DictionaryValue& dict, const char* name, long long ll) {
  dict.SetInteger(name, static_cast<int>(ll));
}
inline void ToArg(base::DictionaryValue& dict, const char* name, const gfx::Size& sz) {
  std::unique_ptr<base::DictionaryValue> d(new base::DictionaryValue());
  d->SetInteger("width", sz.width());
  d->SetInteger("height", sz.height());
  dict.Set(name, std::move(d));
}
inline void ToArg(base::DictionaryValue& dict, const char* name, const gfx::Rect& r) {
  std::unique_ptr<base::DictionaryValue> d(new base::DictionaryValue());
  d->SetInteger("x", r.x());
  d->SetInteger("y", r.y());
  d->SetInteger("width", r.width());
  d->SetInteger("height", r.height());
  dict.Set(name, std::move(d));
}
inline void ToArg(base::DictionaryValue& dict, const char* name, const std::vector<std::string>& strings) {
  std::unique_ptr<base::ListValue> l(new base::ListValue());
  l->AppendStrings(strings);
  dict.Set(name, std::move(l));
}
inline void ToArg(base::DictionaryValue& dict, const char* name, const std::vector<base::string16>& strings) {
  std::unique_ptr<base::ListValue> l(new base::ListValue());
  l->AppendStrings(strings);
  dict.Set(name, std::move(l));
}
inline void ToArg(base::DictionaryValue& dict, const char* name, const base::DictionaryValue* d) {
  dict.Set(name, d->DeepCopy());
}
template <typename First, typename... Rest>
inline void ToArgs(base::DictionaryValue& dict, const char* name, First&& f, Rest&&... rest) {
  ToArg(dict, name, std::forward<First>(f));
  ToArgs(dict, std::move(rest)...);
}
}
}
