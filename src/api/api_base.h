// -*-c++-*-
#pragma once

#include <memory>
#include "base/callback.h"
#include "base/values.h"
#include "base/memory/ref_counted.h"

namespace meson {
class APIBinding;
namespace api {
typedef unsigned int ObjID;
typedef unsigned int ActionID;
typedef base::ListValue APIArgs;
struct CommandArg : base::RefCountedThreadSafe<CommandArg> {
  std::unique_ptr<base::DictionaryValue> arg;
};
struct MethodResultBody : public base::RefCountedThreadSafe<MethodResultBody> {
  std::string error_;
  std::unique_ptr<base::Value> value_;
  scoped_refptr<APIBinding> instance_;
  MethodResultBody() {}
  explicit MethodResultBody(std::unique_ptr<base::Value>&& value)
      : value_(std::move(value)) {}
  explicit MethodResultBody(const std::string& err)
      : error_(err) {}
  explicit MethodResultBody(const char* err)
      : error_(err) {}
  explicit MethodResultBody(scoped_refptr<APIBinding> instance)
      : instance_(instance) {}
  inline bool IsError(void) const { return !error_.empty(); }
  DISALLOW_COPY_AND_ASSIGN(MethodResultBody);
};
struct MethodResult {
  scoped_refptr<MethodResultBody> body_;
  MethodResult()
      : body_(new MethodResultBody()) {}
  explicit MethodResult(std::unique_ptr<base::Value>&& value)
      : body_(new MethodResultBody(std::move(value))) {}
  explicit MethodResult(const std::string& err)
      : body_(new MethodResultBody(err)) {}
  explicit MethodResult(const char* err)
      : body_(new MethodResultBody(err)) {}
  explicit MethodResult(scoped_refptr<APIBinding> instance)
      : body_(new MethodResultBody(instance)) {}
  inline bool IsError(void) const { return !body_->IsError(); }
};

struct EventArg : public base::RefCountedThreadSafe<EventArg> {
  std::string name_;
  std::unique_ptr<base::DictionaryValue> event_;
  EventArg(const std::string& name, std::unique_ptr<base::DictionaryValue> event)
      : name_(name), event_(std::move(event)) {}
  DISALLOW_COPY_AND_ASSIGN(EventArg);
};
//typedef base::DictionaryValue APIEventArgs;
typedef base::Callback<void(scoped_refptr<MethodResultBody> result)> MethodCallback;
typedef base::Callback<void(scoped_refptr<EventArg> event)> EventCallback;
}
}
