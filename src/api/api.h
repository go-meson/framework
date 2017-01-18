//-*-c++-*-
#pragma once
#include <string>
#include <memory>
#include <map>
#include <array>
#include "api/api_base.h"
#include "api/api_binding.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "api/meson.h"
#include "base/synchronization/read_write_lock.h"

namespace meson {
class MesonAppBinding;
class API {
 public:
  API(void);
  virtual ~API(void);

 public:
  static API* Get(void);

  void InstallBindings(MESON_OBJECT_TYPE objType, APIBindingFactory* factory);
  scoped_refptr<APIBinding> CreateAppBinding();
  scoped_refptr<APIBinding> Create(MESON_OBJECT_TYPE type, const api::APICreateArg& args);
  template <typename T>
  scoped_refptr<T> Create(MESON_OBJECT_TYPE type, const api::APICreateArg& args) {
    auto ret = this->Create(type, std::move(args));
    if (!ret) {
      return scoped_refptr<T>();
    }
    return static_cast<T*>(ret.get());
  }
  scoped_refptr<APIBinding> GetBinding(unsigned int target);
  template <typename T>
  scoped_refptr<T> GetBinding(unsigned int target) {
    auto ret = this->GetBinding(target);
    if (!ret) {
      return scoped_refptr<T>();
    }
    return static_cast<T*>(ret.get());
  }
  template <typename T>
  scoped_refptr<T> FindBinding(std::function<bool(const T&)> f) {
    scoped_refptr<T> r;
    EnumBinding<T>([&r, f](T* p) {
      if (f(*p)) {
        r = p;
        return false;
      }
      return true;
    });
    return r;
  }
  template <typename T>
  void EnumBinding(std::function<bool(T*)> f) {
    for (auto& p : bindings_) {
      auto w = p.second;
      if (!w) {
        continue;
      }
      auto s = w.get();
      if (s && (s->type() == T::ObjType)) {
        T* pp = static_cast<T*>(s);
        if (pp) {
          if (!f(pp)) {
            break;
          }
        }
      }
    }
  }
  void CallMethod(unsigned int target, std::string method, std::unique_ptr<const api::APIArgs> args, const api::MethodCallback& callback);
  void SetRemote(unsigned int target, APIBindingRemote* remote);
  void RemoveRemote(unsigned int target, APIBindingRemote* remote);
  void RemoveBinding(APIBinding* binding);
  scoped_refptr<APIBindingRemoteList> GetRemote(unsigned int target);

  int GetNewBindingID(void) { return ++next_binding_id_; }

 private:
  void setBinding(unsigned int id, APIBinding* binding);

 private:
  static API* self_;
  int next_binding_id_;
  mutable base::subtle::ReadWriteLock apiLock_;
  std::array<std::unique_ptr<APIBindingFactory>, MESON_OBJECT_TYPE_NUM> factories_;
  std::map<unsigned int, base::WeakPtr<APIBinding>> bindings_;
  std::map<unsigned int, scoped_refptr<APIBindingRemoteList>> remotes_;
};
}
