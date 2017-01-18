#include "api/api.h"
#include "base/logging.h"
#include "api/api_binding.h"
#include "api/app_binding.h"
#include "api/meson.h"

namespace meson {
API* API::self_ = nullptr;
API::API()
    : next_binding_id_(0) {
  CHECK(!self_) << "API already created";
  self_ = this;
}
API::~API() {}

API* API::Get() {
  return self_;
}

void API::InstallBindings(MESON_OBJECT_TYPE type, APIBindingFactory* factory) {
  DLOG(INFO) << "[API] INSTALL: " << type;
  factories_[type] = std::unique_ptr<APIBindingFactory>(factory);
}

void API::setBinding(unsigned int id, APIBinding* binding) {
  base::subtle::AutoWriteLock l(apiLock_);
  bindings_[id] = base::AsWeakPtr(binding);
}

scoped_refptr<APIBinding> API::CreateAppBinding() {
  base::subtle::AutoWriteLock l(apiLock_);
  if (bindings_.empty()) {
    scoped_refptr<APIBinding> binding = new MesonAppBinding(MESON_OBJID_APP);
    bindings_[MESON_OBJID_APP] = base::AsWeakPtr(binding.get());
    return binding;
  } else {
    return bindings_[MESON_OBJID_APP].get();
  }
}

scoped_refptr<APIBinding> API::Create(MESON_OBJECT_TYPE type, const api::APICreateArg& args) {
  int target_id = 0;
  if (!factories_[type]) {
    LOG(ERROR) << "[API] CREATE: " << type << " is not registerd.";
    return nullptr;
  }
  target_id = GetNewBindingID();
  DLOG(INFO) << "[API] CREATE: " << type << " : " << target_id;
  scoped_refptr<APIBinding> binding(factories_[type]->Create(target_id, args));
  setBinding(target_id, binding.get());
  return binding;
}

scoped_refptr<APIBinding> API::GetBinding(unsigned int target) {
  base::subtle::AutoReadLock l(apiLock_);
  auto fiter = bindings_.find(target);
  if (fiter != bindings_.end()) {
    auto weak = (*fiter).second;
    if (weak.get()) {
      return make_scoped_refptr(weak.get());
    }
  }
  return nullptr;
}
void API::CallMethod(unsigned int target, std::string method, std::unique_ptr<const api::APIArgs> args, const api::MethodCallback& callback) {
  auto binding = GetBinding(target);
  if (!binding) {
    callback.Run("[API] CALL: invalid target no", base::Value::CreateNullValue());
    return;
  }
  DLOG(INFO) << "[API] CALL: " << target << " " << method;
  /* We route the request to the right binding. */
  binding->CallLocalMethod(method, *args, callback);
}

void API::SetRemote(unsigned int target, APIBindingRemote* remote) {
  base::subtle::AutoWriteLock l(apiLock_);
  auto fiter = remotes_.find(target);
  if (fiter == remotes_.end()) {
    remotes_[target] = new APIBindingRemoteList;
  }
  remotes_[target]->AddRemote(remote);
}

void API::RemoveRemote(unsigned int target, meson::APIBindingRemote* remote) {
  base::subtle::AutoWriteLock l(apiLock_);
  LOG(INFO) << __PRETTY_FUNCTION__;
  auto fiter = remotes_.find(target);
  if (fiter != remotes_.end()) {
    if ((*fiter).second->RemoveRemote(remote)) {
      remotes_.erase(fiter);
    }
  }
}

void API::RemoveBinding(meson::APIBinding* binding) {
  LOG(INFO) << __PRETTY_FUNCTION__ << (binding ? binding->GetID() : 0);
  CHECK(binding);
  int id = binding->GetID();
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

scoped_refptr<APIBindingRemoteList> API::GetRemote(unsigned int target) {
  base::subtle::AutoReadLock l(apiLock_);
  scoped_refptr<APIBindingRemoteList> ret;
  auto fiter = remotes_.find(target);
  if (fiter != remotes_.end()) {
    ret = make_scoped_refptr((*fiter).second.get());
  }
  return ret;
}
}
