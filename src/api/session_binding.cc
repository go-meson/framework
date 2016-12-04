#include "api/session_binding.h"

#include "base/strings/string_util.h"
#include "api/api.h"

namespace {
const char kPersistPrefix[] = "persist:";
}

namespace meson {
template <>
APIBindingT<MesonSessionBinding>::MethodTable APIBindingT<MesonSessionBinding>::methodTable = {};

MesonSessionBinding::MesonSessionBinding(unsigned int id, const api::APICreateArg& args)
    : APIBindingT(MESON_OBJECT_TYPE_SESSION, id) {
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
MesonSessionBinding::~MesonSessionBinding(void) {}

scoped_refptr<MesonBrowserContext> MesonSessionBinding::GetSession() const {
  return browser_context_;
}
void MesonSessionBinding::CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  DCHECK(FALSE) << "not implement yet";
}

MesonSessionFactory::MesonSessionFactory(void) {}
MesonSessionFactory::~MesonSessionFactory(void) {}
APIBinding* MesonSessionFactory::Create(unsigned int id, const api::APICreateArg& args) {
  return new MesonSessionBinding(id, args);
}
}
