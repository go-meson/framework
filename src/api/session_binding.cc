#include "api/session_binding.h"

#include "base/strings/string_util.h"
#include "api/api.h"

namespace {
const char kPersistPrefix[] = "persist:";
}

namespace meson {
template <>
const APIBindingT<SessionBinding, SessionClassBinding>::MethodTable APIBindingT<SessionBinding, SessionClassBinding>::methodTable = {};

template <>
const APIClassBindingT<SessionBinding, SessionClassBinding>::MethodTable APIClassBindingT<SessionBinding, SessionClassBinding>::staticMethodTable = {
    {"_create", std::mem_fn(&SessionClassBinding::CreateInstance)},
};

MESON_IMPLEMENT_API_CLASS(SessionBinding, SessionClassBinding);

// TODO: constructorと初期化を分割すべきか？
SessionBinding::SessionBinding(api::ObjID id, const base::DictionaryValue& args)
    : APIBindingT(MESON_OBJECT_TYPE_SESSION, id) {
  if (id == MESON_OBJID_STATIC) {
    return;
  }
  std::string partition;
  args.GetString("partition", &partition);
  const base::DictionaryValue* opt = nullptr;
  base::DictionaryValue dummy;
  args.GetDictionary("options", &opt);
  if (!opt) {
    opt = &dummy;
  }

  if (partition.empty()) {
    browser_context_ = MesonBrowserContext::From(this, "", false, *opt);
  } else if (base::StartsWith(partition, kPersistPrefix, base::CompareCase::SENSITIVE)) {
    std::string name = partition.substr(sizeof(kPersistPrefix) - 1);
    browser_context_ = MesonBrowserContext::From(this, name, false, *opt);
  } else {
    browser_context_ = MesonBrowserContext::From(this, partition, true, *opt);
  }
}
SessionBinding::~SessionBinding(void) {}

scoped_refptr<MesonBrowserContext> SessionBinding::GetSession() const {
  return browser_context_;
}

SessionClassBinding::SessionClassBinding(void)
    : APIClassBindingT(MESON_OBJECT_TYPE_SESSION) {}
SessionClassBinding::~SessionClassBinding(void) {}

scoped_refptr<SessionBinding> SessionClassBinding::NewInstance(const base::DictionaryValue& opt) {
  auto id = GetNextBindingID();
  scoped_refptr<SessionBinding> binding(new SessionBinding(id, opt));
  SetBinding(id, binding);
  return binding;
}

api::MethodResult SessionClassBinding::CreateInstance(const api::APIArgs& args) {
  const base::DictionaryValue* opt;
  args.GetDictionary(0, &opt);
  return api::MethodResult(NewInstance(*opt));
}
}
