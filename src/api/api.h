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

namespace meson {
class API {
 public:
  API(void);
  virtual ~API(void);

 public:
  static API* Get(void);

  void InstallBindings(MESON_OBJECT_TYPE objType, APIBindingFactory* factory);
  void CreateAppBinding();
  scoped_refptr<APIBinding> Create(MESON_OBJECT_TYPE type, const api::APICreateArg& args);
  template<typename T>
  scoped_refptr<T> Create(MESON_OBJECT_TYPE type, const api::APICreateArg& args) {
    auto ret = this->Create(type, std::move(args));
    if (!ret) {
      return scoped_refptr<T>();
    }
    return static_cast<T*>(ret.get());
  }
  scoped_refptr<APIBinding> GetBinding(unsigned int target);
  template<typename T>
  scoped_refptr<T> GetBinding(unsigned int target) {
    auto ret = this->GetBinding(target);
    if (!ret) {
      return scoped_refptr<T>();
    }
    return static_cast<T*>(ret.get());
  }
  void CallMethod(unsigned int target, std::string method, std::unique_ptr<const api::APIArgs> args, const api::MethodCallback& callback);
  void SetRemote(unsigned int target, scoped_refptr<APIBindingRemote> remote);
  APIBindingRemote* GetRemote(unsigned int target);

  int GetNewBindingID(void) { return ++next_binding_id_; }

 private:
  static API* self_;
  int next_binding_id_;
  std::array<std::unique_ptr<APIBindingFactory>, MESON_OBJECT_TYPE_NUM> factories_;
  std::map<unsigned int, scoped_refptr<APIBinding>> bindings_;
  std::map<unsigned int, scoped_refptr<APIBindingRemote>> remotes_;
};
}
