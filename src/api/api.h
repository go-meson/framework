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
//TODO: split API?
class API {
 public:
  class BindingResolver : public base::RefCounted<BindingResolver> {
   public:
    virtual ~BindingResolver(void) {}

    virtual scoped_refptr<APIBinding> GetBinding(api::ObjID id) = 0;
  };
  template <typename T>
  class BindingResolverT : public BindingResolver {
   public:
    BindingResolverT(T* binding)
        : klass_(binding) {}
    ~BindingResolverT(void) override {}

   public:
    virtual scoped_refptr<APIBinding> GetBinding(api::ObjID id) {
      if (id == MESON_OBJID_STATIC) {
        return klass_;
      } else {
        return klass_->GetBinding(id);
      }
    }
    scoped_refptr<T> klass_;
  };

 public:
  API(void);
  virtual ~API(void);

 public:
  static API* Get(void);

  template <typename T>
  void InstallBindings(MESON_OBJECT_TYPE objType, T* classBinding) {
    klasses_[objType] = new BindingResolverT<T>(classBinding);
  }
  APIBinding* GetBinding(MESON_OBJECT_TYPE type, api::ObjID id) const {
    auto klass = klasses_.at(type);
    if (!klass)
      return nullptr;
    return klass->GetBinding(id).get();
  }

 private:
  static API* self_;
  //TODO: wrap struct??
  std::array<scoped_refptr<BindingResolver>, MESON_OBJECT_TYPE_NUM> klasses_;
};
}
