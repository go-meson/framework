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

void API::CreateAppBinding() {
  if (bindings_.empty()) {
    bindings_[MESON_OBJID_APP] = new MesonAppBinding(MESON_OBJID_APP);
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
  bindings_[target_id] = binding;
  return binding;
}

scoped_refptr<APIBinding> API::GetBinding(unsigned int target) {
  if (target > 0) {
    auto fiter = bindings_.find(target);
    if (fiter != bindings_.end()) {
      auto weak = (*fiter).second;
      if (weak.get()) {
        return make_scoped_refptr(weak.get());
      }
    }
  }
  return nullptr;
}
void API::CallMethod(unsigned int target, std::string method, std::unique_ptr<const api::APIArgs> args, const api::MethodCallback& callback) {
  auto fiter = bindings_.find(target);
  if (fiter != bindings_.end()) {
    DLOG(INFO) << "[API] CALL: " << target << " " << method;
    /* We route the request to the right binding. */
    (*fiter).second->CallLocalMethod(method, *args, callback);
  } else {
    callback.Run("[API] CALL: invalid target no", base::Value::CreateNullValue());
  }
}

void API::SetRemote(unsigned int target, scoped_refptr<APIBindingRemote> remote) {
  auto fiter = remotes_.find(target);
  if (fiter != remotes_.end()) {
    DCHECK(false);
    return;
  }
  remotes_[target] = remote;
}
APIBindingRemote* API::GetRemote(unsigned int target) {
  auto fiter = remotes_.find(target);
  if (fiter != remotes_.end()) {
    return (*fiter).second.get();
  }
  return nullptr;
}
}
