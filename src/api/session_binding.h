//-*-c++-*-
#pragma once
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "api/api_binding.h"
#include "browser/session/meson_browser_context.h"

namespace meson {
class MesonBrowserContext;
class MesonSessionBinding : public APIBindingT<MesonSessionBinding> {
 public:
  MesonSessionBinding(unsigned int id, const api::APICreateArg& args);
  virtual ~MesonSessionBinding(void);

 public:
  scoped_refptr<MesonBrowserContext> GetSession() const;

 public:
  virtual void CallLocalMethod(const std::string& method, const api::APIArgs& args, const api::MethodCallback& callback) override;

 private:
  scoped_refptr<MesonBrowserContext> browser_context_;
  DISALLOW_COPY_AND_ASSIGN(MesonSessionBinding);
};

class MesonSessionFactory : public APIBindingFactory {
 public:
  MesonSessionFactory(void);
  virtual ~MesonSessionFactory(void);

 public:
  virtual APIBinding* Create(unsigned int id, const api::APICreateArg& args) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MesonSessionFactory);
};
}
