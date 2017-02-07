//-*-c++-*-
#pragma once
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "api/api_binding.h"
#include "browser/session/meson_browser_context.h"

namespace meson {
class SessionClassBinding;
class MesonBrowserContext;
class SessionBinding : public APIBindingT<SessionBinding, SessionClassBinding> {
 public:
  SessionBinding(api::ObjID id, const base::DictionaryValue& args);
  virtual ~SessionBinding(void);

 public:
  scoped_refptr<MesonBrowserContext> GetSession() const;

 private:
  scoped_refptr<MesonBrowserContext> browser_context_;
  DISALLOW_COPY_AND_ASSIGN(SessionBinding);
};

class SessionClassBinding : public APIClassBindingT<SessionBinding, SessionClassBinding> {
 public:
  SessionClassBinding(void);
  ~SessionClassBinding(void) override;

 public:
  scoped_refptr<SessionBinding> NewInstance(const base::DictionaryValue& opt);

 public:  // static methods
  api::MethodResult CreateInstance(const api::APIArgs& args);

 private:
  DISALLOW_COPY_AND_ASSIGN(SessionClassBinding);
};
}
