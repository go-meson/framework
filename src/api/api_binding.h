//-*-c++-*-
#pragma once

#include <string>
#include <memory>
#include <map>
#include <tuple>
#include <functional>
#include <exception>
#include <list>
#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/read_write_lock.h"
#include "api/api_base.h"
#include "api/meson.h"
#include "api/api_binding_helper.h"

namespace meson {
class API;
class APIBinding;
class APIBindingRemote;
class APIBindingRemoteList;
template <typename T, typename TC>
class APIClassBindingT;

class APIBinding
    : public base::RefCountedThreadSafe<APIBinding>,
      public base::SupportsWeakPtr<APIBinding> {
 protected:
 public:
  virtual ~APIBinding(void);

 protected:
  APIBinding(MESON_OBJECT_TYPE type, api::ObjID id);

 public:
  virtual void CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) = 0;
  MESON_OBJECT_TYPE Type(void) const { return type_; }
  inline api::ObjID GetID() const { return id_; }
  std::unique_ptr<base::DictionaryValue> GetTypeID(void) const {
    std::unique_ptr<base::DictionaryValue> ret(new base::DictionaryValue());
    ret->SetInteger("_type", static_cast<int>(Type()));
    ret->SetInteger("_id", static_cast<int>(GetID()));
    return ret;
  }
  virtual scoped_refptr<APIBindingRemoteList> GetRemote(api::ObjID id) const = 0;
  virtual void SetRemote(api::ObjID id, APIBindingRemote* binding) = 0;

  template <typename... T>
  void EmitEvent(const std::string& event_type, T... args) {
    std::unique_ptr<base::DictionaryValue> event(new base::DictionaryValue());
    meson::internal::ToArgs(*event, args...);
    EmitEvent(event_type, std::move(event));
  }
  template <typename... T>
  bool EmitPreventEvent(const std::string& event_type, T... args) {
    std::unique_ptr<base::DictionaryValue> event(new base::DictionaryValue());
    meson::internal::ToArgs(*event, args...);
    auto result = EmitEventWithResult(event_type, std::move(event));
    bool ret = false;
    if (result) {
      result->GetAsBoolean(&ret);
    }
    return ret;
  }

 protected:
  void InvokeRemoteMethod(const std::string& method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback);
  void EmitEvent(const std::string& event_type, std::unique_ptr<base::DictionaryValue> event);
  std::unique_ptr<base::Value> EmitEventWithResult(const std::string& event_type, std::unique_ptr<base::DictionaryValue> event);

 protected:
  MESON_OBJECT_TYPE type_;
  api::ObjID id_;
  DISALLOW_COPY_AND_ASSIGN(APIBinding);
};

template <typename T, typename TC>
class APIBindingT : public APIBinding {
 protected:
  typedef std::map<std::string, std::function<meson::api::MethodResult(T*, const api::APIArgs&)>> MethodTable;

 public:
  void CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) override;
  scoped_refptr<APIBindingRemoteList> GetRemote(api::ObjID id) const override;
  void SetRemote(api::ObjID id, APIBindingRemote* remote) override;
  static TC& Class(void);

 protected:
  APIBindingT(MESON_OBJECT_TYPE type, api::ObjID id)
      : APIBinding(type, id) {}

 private:
  static const MethodTable methodTable;
};

class APIBindingRemote
    : public base::RefCountedThreadSafe<APIBindingRemote>,
      public base::SupportsWeakPtr<APIBindingRemote> {
 public:
  virtual ~APIBindingRemote(void) {}

 public:
  virtual void RemoveBinding(APIBinding* binding) = 0;

 public:
  virtual bool InvokeMethod(const std::string method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback) = 0;
  virtual void EmitEvent(scoped_refptr<api::EventArg> event) = 0;
  virtual std::unique_ptr<base::Value> EmitEventWithResult(scoped_refptr<api::EventArg> event) = 0;

 public:
  APIBinding* Binding() {
    return binding_.get();
  }
  template <typename T>
  T* Binding() {
    return static_cast<T*>(Binding());
  }
  bool HasBinding(APIBinding* binding) const {
    return binding_.get() == binding;
  }

 protected:
  APIBindingRemote(scoped_refptr<APIBinding> binding)
      : binding_(binding) {}

  scoped_refptr<APIBinding> binding_;

 private:
  DISALLOW_COPY_AND_ASSIGN(APIBindingRemote);
};

class APIBindingRemoteList : public base::RefCountedThreadSafe<APIBindingRemoteList> {
 public:
  APIBindingRemoteList(void);
  ~APIBindingRemoteList(void);

 public:
  void AddRemote(APIBindingRemote* remote);
  bool RemoveRemote(APIBindingRemote* remote);
  void RemoveBinding(APIBinding* binding);

 public:
  void InvokeMethod(const std::string method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback);
  void EmitEvent(scoped_refptr<api::EventArg> event);
  std::unique_ptr<base::Value> EmitEventWithResult(scoped_refptr<api::EventArg> event);

 private:
  std::list<scoped_refptr<APIBindingRemote>> remotes_;
  DISALLOW_COPY_AND_ASSIGN(APIBindingRemoteList);
};

template <typename T, typename TC>
class APIClassBindingT : public APIBindingT<T, TC> {
 protected:
  typedef std::map<std::string, std::function<meson::api::MethodResult(TC*, const api::APIArgs&)>> MethodTable;

 public:
  static TC* Get() { return self_; }

 protected:
  APIClassBindingT(MESON_OBJECT_TYPE type)
      : APIBindingT<T, TC>(type, MESON_OBJID_STATIC) {
    self_ = static_cast<TC*>(this);
  }
  ~APIClassBindingT() override {
    self_ = nullptr;
  }

 public:
  int GetNextBindingID(void) { return ++next_binding_id_; }
  void EnumBinding(std::function<bool(T*)> f);
  scoped_refptr<T> FindBinding(std::function<bool(const T&)> f);
  void CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) override;
  scoped_refptr<T> GetBinding(api::ObjID id);
  void SetBinding(api::ObjID id, scoped_refptr<T> binding);
  void SetRemote(api::ObjID, APIBindingRemote* remote);
  scoped_refptr<APIBindingRemoteList> GetRemote(api::ObjID id) const override;
  void RemoveRemote(api::ObjID id, APIBindingRemote* remote);
  void RemoveBinding(APIBinding* binding);

 protected:
  static TC* self_;
  int next_binding_id_;
  std::map<api::ObjID, scoped_refptr<APIBindingRemoteList>> remotes_;
  std::map<api::ObjID, base::WeakPtr<T>> bindings_;
  mutable base::subtle::ReadWriteLock apiLock_;
  static const MethodTable staticMethodTable;
};

#define MESON_IMPLEMENT_API_CLASS(T, TC) \
  template <>                            \
  TC* APIClassBindingT<T, TC>::self_ = nullptr;

template <typename T, typename TC>
void APIBindingT<T, TC>::CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) {
  auto fiter = methodTable.find(method);
  if (fiter == methodTable.end()) {
    callback.Run(make_scoped_refptr(new api::MethodResultBody(std::string("CALL : unknown method: ") + method)));
    return;
  }
  auto&& result = (*fiter).second(static_cast<T*>(this), args);
  callback.Run(result.body_);
}

template <typename T, typename TC>
TC& APIBindingT<T, TC>::Class() {
  return *TC::Get();
}

template <typename T, typename TC>
scoped_refptr<APIBindingRemoteList> APIBindingT<T, TC>::GetRemote(api::ObjID id) const {
  return Class().GetRemote(id);
}
template <typename T, typename TC>
void APIBindingT<T, TC>::SetRemote(api::ObjID id, meson::APIBindingRemote* remote) {
  Class().SetRemote(id, remote);
}

template <typename T, typename TC>
scoped_refptr<APIBindingRemoteList> APIClassBindingT<T, TC>::GetRemote(api::ObjID id) const {
  base::subtle::AutoReadLock l(apiLock_);
  scoped_refptr<APIBindingRemoteList> ret;
  auto fiter = remotes_.find(id);
  if (fiter != remotes_.end()) {
    ret = (*fiter).second;
  }
  return ret;
}

template <typename T, typename TC>
void APIClassBindingT<T, TC>::SetRemote(api::ObjID id, meson::APIBindingRemote* remote) {
  base::subtle::AutoWriteLock l(apiLock_);
  scoped_refptr<APIBindingRemoteList> lst;
  auto fiter = remotes_.find(id);
  if (fiter != remotes_.end()) {
    lst = (*fiter).second;
  } else {
    lst = new APIBindingRemoteList();
    remotes_[id] = lst;
  }
  lst->AddRemote(remote);
}

template <typename T, typename TC>
void APIClassBindingT<T, TC>::EnumBinding(std::function<bool(T*)> f) {
  for (auto& kv : bindings_) {
    auto w = kv.second;
    if (!w)
      continue;
    auto s = w.get();
    if (s) {
      T* pp = static_cast<T*>(s);
      if (!f(pp)) {
        break;
      }
    }
  }
}

template <typename T, typename TC>
void APIClassBindingT<T, TC>::CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) {
  auto fiter = staticMethodTable.find(method);
  if (fiter == staticMethodTable.end()) {
    callback.Run(make_scoped_refptr(new api::MethodResultBody(std::string("CALL[static] : unknown method: ") + method)));
    return;
  }
  auto&& result = (*fiter).second(static_cast<TC*>(this), args);
  callback.Run(result.body_);
}

template <typename T, typename TC>
scoped_refptr<T> APIClassBindingT<T, TC>::GetBinding(api::ObjID id) {
  base::subtle::AutoReadLock l(apiLock_);
  auto fiter = bindings_.find(id);
  if (fiter != bindings_.end()) {
    auto weak = (*fiter).second;
    if (weak.get()) {
      return make_scoped_refptr(weak.get());
    }
  }
  return nullptr;
}

template <typename T, typename TC>
void APIClassBindingT<T, TC>::SetBinding(api::ObjID id, scoped_refptr<T> binding) {
  base::subtle::AutoReadLock l(apiLock_);
  bindings_[id] = base::AsWeakPtr(binding.get());
}

template <typename T, typename TC>
scoped_refptr<T> APIClassBindingT<T, TC>::FindBinding(std::function<bool(const T&)> f) {
  scoped_refptr<T> r;
  EnumBinding([&r, f](T* p) {
    if (f(*p)) {
      r = p;
      return false;
    }
    return true;
  });
  return r;
}

template <typename T, typename TC>
void APIClassBindingT<T, TC>::RemoveRemote(api::ObjID id, APIBindingRemote* remote) {
  base::subtle::AutoWriteLock l(apiLock_);
  auto fiter = remotes_.find(id);
  if (fiter != remotes_.end()) {
    if ((*fiter).second->RemoveRemote(remote)) {
      remotes_.erase(fiter);
    }
  }
}

template <typename T, typename TC>
void APIClassBindingT<T, TC>::RemoveBinding(APIBinding* binding) {
  auto id = binding->GetID();
  scoped_refptr<APIBindingRemoteList> remote_list;
  {
    base::subtle::AutoWriteLock l(apiLock_);
    {
      auto fiter = bindings_.find(id);
      if (fiter != bindings_.end()) {
        bindings_.erase(fiter);
      }
    }
    {
      auto fiter = remotes_.find(id);
      if (fiter != remotes_.end()) {
        remote_list = (*fiter).second;
        remotes_.erase(fiter);
      }
    }
  }
  if (remote_list) {
    remote_list->RemoveBinding(binding);
  }
}
}
