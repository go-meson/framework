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
}
