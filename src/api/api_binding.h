//-*-c++-*-
#pragma once

#include <string>
#include <memory>
#include <map>
#include <tuple>
#include <functional>
#include <exception>
#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "api/api_base.h"
#include "api/meson.h"
#include "url/gurl.h"
#include "ui/gfx/geometry/rect.h"

namespace meson {
namespace internal {
inline void ToArgs(base::ListValue& list) {}
inline void ToArg(base::ListValue& list, int a) {
  list.AppendInteger(a);
}
inline void ToArg(base::ListValue& list, double d) {
  list.AppendDouble(d);
}
inline void ToArg(base::ListValue& list, bool b) {
  list.AppendBoolean(b);
}
inline void ToArg(base::ListValue& list, const std::string& s) {
  list.AppendString(s);
}
inline void ToArg(base::ListValue& list, base::string16& s) {
  list.AppendString(s);
}
inline void ToArg(base::ListValue& list, const GURL& url) {
  list.AppendString(url.spec());
}
inline void ToArg(base::ListValue& list, long long ll) {
  list.AppendInteger(static_cast<int>(ll));
}
inline void ToArg(base::ListValue& list, const gfx::Rect& r) {
  std::unique_ptr<base::DictionaryValue> d(new base::DictionaryValue());
  d->SetInteger("x", r.x());
  d->SetInteger("y", r.y());
  d->SetInteger("width", r.width());
  d->SetInteger("height", r.height());
  list.Append(std::move(d));
}
inline void ToArg(base::ListValue& list, std::vector<base::string16>& strings) {
  std::unique_ptr<base::ListValue> l(new base::ListValue());
  l->AppendStrings(strings);
  list.Append(std::move(l));
}
inline void ToArg(base::ListValue& list, const base::DictionaryValue* d) {
  list.Append(d->DeepCopy());
}
template <typename First, typename... Rest>
inline void ToArgs(base::ListValue& list, First&& f, Rest&&... rest) {
  ToArg(list, f);
  ToArgs(list, std::move(rest)...);
}
}
class APIBinding : public base::RefCountedThreadSafe<APIBinding> {
 protected:
  struct MethodResult {
    std::string error_;
    std::unique_ptr<base::Value> value_;
    MethodResult() {}
    explicit MethodResult(std::unique_ptr<base::Value>&& value)
        : value_(std::move(value)) {}
    explicit MethodResult(const std::string& err)
        : error_(err) {}
    explicit MethodResult(const char* err)
        : error_(err) {}
    inline bool IsError(void) const { return !error_.empty(); }
  };

 public:
  virtual ~APIBinding(void);

 protected:
  APIBinding(MESON_OBJECT_TYPE type, unsigned int id);

 public:
  virtual void CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) = 0;
  MESON_OBJECT_TYPE type(void) const { return type_; }
  unsigned int GetID() const { return id_; }

 protected:
  void InvokeRemoteMethod(const std::string& method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback);
  void EmitEvent(const std::string& event_type, std::unique_ptr<base::ListValue> event);
  std::unique_ptr<base::Value> EmitEventWithResult(const std::string& event_type, std::unique_ptr<base::ListValue> event);

  template <typename... T>
  void EmitEvent(const std::string& event_type, T... args) {
    std::unique_ptr<base::ListValue> event(new base::ListValue());
    meson::internal::ToArgs(*event, args...);
    EmitEvent(event_type, std::move(event));
  }
  template <typename... T>
  bool EmitPreventEvent(const std::string& event_type, T... args) {
    std::unique_ptr<base::ListValue> event(new base::ListValue());
    meson::internal::ToArgs(*event, args...);
    auto result = EmitEventWithResult(event_type, std::move(event));
    bool ret = false;
    if (result) {
      result->GetAsBoolean(&ret);
    }
    return ret;
  }

 protected:
  MESON_OBJECT_TYPE type_;
  unsigned int id_;
  DISALLOW_COPY_AND_ASSIGN(APIBinding);
};

template <typename T>
class APIBindingT : public APIBinding {
 protected:
  typedef std::map<std::string, std::function<MethodResult(T*, const api::APIArgs&)>> MethodTable;

 public:
  base::WeakPtr<T> GetWeakPtr(void) { return weak_ptr_factory_.GetWeakPtr(); }
  void CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) override {
    auto fiter = methodTable.find(method);
    if (fiter == methodTable.end()) {
      callback.Run(std::string("CALL : unknown method: ") + method, std::unique_ptr<base::Value>());
      return;
    }
    auto result = std::move((*fiter).second((T*)this, args));
    callback.Run(result.error_, std::move(result.value_));
  }

 protected:
  base::WeakPtrFactory<T> weak_ptr_factory_;
  APIBindingT(MESON_OBJECT_TYPE type, unsigned int id)
      : APIBinding(type, id), weak_ptr_factory_(static_cast<T*>(this)) {}
  static MethodTable methodTable;
};

class APIBindingFactory {
 public:
  virtual ~APIBindingFactory(void) {}

 protected:
  APIBindingFactory(void) {}

 public:
  virtual APIBinding* Create(unsigned int id, const api::APICreateArg& args) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(APIBindingFactory);
};

class APIBindingRemote : public base::RefCountedThreadSafe<APIBindingRemote> {
 public:
  virtual ~APIBindingRemote(void) {}

 public:
  virtual void InvokeMethod(const std::string method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback) = 0;
  virtual void EmitEvent(const std::string& type, std::unique_ptr<base::ListValue> event) = 0;
  virtual std::unique_ptr<base::Value> EmitEventWithResult(const std::string& type, std::unique_ptr<base::ListValue> event) = 0;

 protected:
  APIBindingRemote() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(APIBindingRemote);
};
}
