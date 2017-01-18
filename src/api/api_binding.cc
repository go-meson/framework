#include "api/api_binding.h"
#include "api/api.h"

namespace meson {
APIBinding::APIBinding(MESON_OBJECT_TYPE type, unsigned int id)
    : type_(type), id_(id) {}

APIBinding::~APIBinding(void) {}

void APIBinding::InvokeRemoteMethod(const std::string& method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback) {
  auto remote = API::Get()->GetRemote(id_);
  if (remote) {
    remote->InvokeMethod(method, std::move(args), callback);
  }
}
void APIBinding::EmitEvent(const std::string& type, std::unique_ptr<base::ListValue> event) {
  auto remote = API::Get()->GetRemote(id_);
  DLOG(INFO) << __PRETTY_FUNCTION__ << " : " << (int64_t)id_ << ", " << *event;
  if (remote) {
    remote->EmitEvent(type, std::move(event));
  }
}
std::unique_ptr<base::Value> APIBinding::EmitEventWithResult(const std::string& event_type, std::unique_ptr<base::ListValue> event) {
  auto remote = API::Get()->GetRemote(id_);
  DLOG(INFO) << __PRETTY_FUNCTION__ << " : " << (int64_t)id_ << ", " << *event;
  if (!remote) {
    return std::unique_ptr<base::Value>();
  }
  return remote->EmitEventWithResult(event_type, std::move(event));
}

APIBindingRemoteList::APIBindingRemoteList() {}
APIBindingRemoteList::~APIBindingRemoteList() {}

void APIBindingRemoteList::AddRemote(meson::APIBindingRemote* remote) {
  auto fiter = std::find_if(remotes_.begin(), remotes_.end(), [remote](const scoped_refptr<APIBindingRemote>& c) {
    return c.get() == remote;
  });
  if (fiter != remotes_.end()) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : remote is already registerd.";
    return;
  }
  remotes_.push_back(remote);
}

bool APIBindingRemoteList::RemoveRemote(meson::APIBindingRemote* remote) {
  auto fiter = std::find_if(remotes_.begin(), remotes_.end(), [remote](const scoped_refptr<APIBindingRemote>& c) {
    return c.get() == remote;
  });
  scoped_refptr<APIBindingRemote> targets;
  if (fiter != remotes_.end()) {
    targets = (*fiter);
    remotes_.erase(fiter);
  }
  return remotes_.empty();
}

void APIBindingRemoteList::RemoveBinding(meson::APIBinding* binding) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  CHECK(binding);
  decltype(remotes_) targets;
  targets.swap(remotes_);
  if (targets.empty()) {
    return;
  }

  for (auto target : targets) {
    target->RemoveBinding(binding);
  }
}

void APIBindingRemoteList::InvokeMethod(const std::string method, std::unique_ptr<api::APIArgs> args, const api::MethodCallback& callback) {
  std::vector<scoped_refptr<APIBindingRemote>> remotes;
  std::copy(remotes_.begin(), remotes_.end(), std::back_inserter(remotes));
  for (auto& remote : remotes) {
    //TODO: unique_ptr渡しを止める
    std::unique_ptr<api::APIArgs> c(args->DeepCopy());
    if (remote->InvokeMethod(method, std::move(c), callback)) {
      return;
    }
  }
}
void APIBindingRemoteList::EmitEvent(const std::string& type, std::unique_ptr<base::ListValue> event) {
  std::vector<scoped_refptr<APIBindingRemote>> remotes;
  std::copy(remotes_.begin(), remotes_.end(), std::back_inserter(remotes));
  for (auto& remote : remotes) {
    //TODO: unique_ptr渡しを止める
    std::unique_ptr<base::ListValue> e(event->DeepCopy());
    remote->EmitEvent(type, std::move(e));
  }
}

std::unique_ptr<base::Value> APIBindingRemoteList::EmitEventWithResult(const std::string& type, std::unique_ptr<base::ListValue> event) {
  std::vector<scoped_refptr<APIBindingRemote>> remotes;
  std::copy(remotes_.begin(), remotes_.end(), std::back_inserter(remotes));
  for (auto& remote : remotes) {
    //TODO: unique_ptr渡しを止める
    std::unique_ptr<base::ListValue> e(event->DeepCopy());
    auto ret = remote->EmitEventWithResult(type, std::move(e));
    if (ret) {
      return ret;
    }
  }
  return std::unique_ptr<base::Value>();
}
}
