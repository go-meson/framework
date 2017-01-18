// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(dcarney): Remove this when UnsafePersistent is removed.
#define V8_ALLOW_ACCESS_TO_RAW_HANDLE_CONSTRUCTOR

#include "renderer/extensions/object_backed_native_handler.h"

#include "base/logging.h"
#include "base/memory/linked_ptr.h"
#include "v8/include/v8.h"

#include "content/public/child/worker_thread.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

#include "renderer/extensions/console.h"
#include "renderer/extensions/script_context.h"
#include "renderer/extensions/v8_helpers.h"

namespace extensions {

namespace {
// Key for the base::Bound routed function.
const char* kHandlerFunction = "handler_function";
const char* kFeatureName = "feature_name";
}  // namespace

ObjectBackedNativeHandler::ObjectBackedNativeHandler(ScriptContext* context)
    : router_data_(context->v8_context()->GetIsolate()),
      context_(context),
      object_template_(context->isolate(), v8::ObjectTemplate::New(context->isolate())) {
  DLOG(INFO) << __PRETTY_FUNCTION__;
}

ObjectBackedNativeHandler::~ObjectBackedNativeHandler() {
  Invalidate();
}

v8::Handle<v8::Object> ObjectBackedNativeHandler::NewInstance() {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  return v8::Local<v8::ObjectTemplate>::New(GetIsolate(), object_template_)->NewInstance();
}

// static
void ObjectBackedNativeHandler::Router(const v8::FunctionCallbackInfo<v8::Value>& args) {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(args.GetIsolate());
  v8::Handle<v8::Object> data = args.Data().As<v8::Object>();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::Value> handler_function_value;
  v8::Local<v8::Value> feature_name_value;
  if (!GetPrivate(context, data, kHandlerFunction, &handler_function_value) ||
      handler_function_value->IsUndefined() ||
      !GetPrivate(context, data, kFeatureName, &feature_name_value) ||
      !feature_name_value->IsString()) {
    ScriptContext* script_context = ScriptContext::FromV8Context(context);
    console::Error(script_context ? script_context->GetRenderView() : nullptr, "Extension view no longer exists");
    return;
  }

  if (content::WorkerThread::GetCurrentId() == 0) {
    ScriptContext* script_context = ScriptContext::FromV8Context(context);
    v8::Local<v8::String> feature_name_string = feature_name_value->ToString(context).ToLocalChecked();
    std::string feature_name = *v8::String::Utf8Value(feature_name_string);
    // TODO(devlin): Eventually, we should fail if either script_context is null
    // or feature_name is empty.
    if (script_context && !feature_name.empty()) {
      //TODO: need implement ScriptContext::GetAvailability??
    }
  }

  CHECK(handler_function_value->IsExternal());
  static_cast<HandlerFunction*>(handler_function_value.As<v8::External>()->Value())->Run(args);

  // Verify that the return value, if any, is accessible by the context.
  v8::ReturnValue<v8::Value> ret = args.GetReturnValue();
  v8::Local<v8::Value> ret_value = ret.Get();
  if (ret_value->IsObject() &&
      !ret_value->IsNull() &&
      !ContextCanAccessObject(context, v8::Local<v8::Object>::Cast(ret_value), true)) {
    NOTREACHED() << "Insecure return value";
    ret.SetUndefined();
  }
}

void ObjectBackedNativeHandler::RouteFunction(const std::string& name, const HandlerFunction& handler_function) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << "(" << name << ")";
  RouteFunction(name, "", handler_function);
}

void ObjectBackedNativeHandler::RouteFunction(const std::string& name, const std::string& feature_name, const HandlerFunction& handler_function) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << "(" << name << ", " << feature_name << ")";
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context_->v8_context());

  v8::Local<v8::Object> data = v8::Object::New(isolate);
  SetPrivate(data, kHandlerFunction, v8::External::New(isolate, new HandlerFunction(handler_function)));

  SetPrivate(data, kFeatureName, v8_helpers::ToV8StringUnsafe(isolate, feature_name));

  v8::Handle<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate, Router, data);
  function_template->RemovePrototype();
  v8::Local<v8::ObjectTemplate>::New(isolate, object_template_)->Set(isolate, name.c_str(), function_template);
  router_data_.Append(data);
}

v8::Isolate* ObjectBackedNativeHandler::GetIsolate() const {
  return context_->isolate();
}

void ObjectBackedNativeHandler::Invalidate() {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << ")";
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context_->v8_context());

  for (size_t i = 0; i < router_data_.Size(); i++) {
    v8::Handle<v8::Object> data = router_data_.Get(i);
    v8::Handle<v8::Value> handler_function_value;
    CHECK(GetPrivate(data, kHandlerFunction, &handler_function_value));
    delete static_cast<HandlerFunction*>(handler_function_value.As<v8::External>()->Value());
    DeletePrivate(data, kHandlerFunction);
  }
  router_data_.Clear();
  object_template_.Reset();

  NativeHandler::Invalidate();
}

bool ObjectBackedNativeHandler::ContextCanAccessObject(const v8::Local<v8::Context>& context,
                                                       const v8::Local<v8::Object>& object,
                                                       bool allow_null_context) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << "(" << ")";
  if (object->IsNull())
    return true;
  if (context == object->CreationContext())
    return true;
  // TODO(lazyboy): ScriptContextSet isn't available on worker threads. We
  // should probably use WorkerScriptContextSet somehow.
  ScriptContext* other_script_context = content::WorkerThread::GetCurrentId() == 0
                                            ? ScriptContext::FromV8Context(object->CreationContext())
                                            : nullptr;
  if (!other_script_context || !other_script_context->web_frame())
    return allow_null_context;

  return blink::WebFrame::scriptCanAccess(other_script_context->web_frame());
}

void ObjectBackedNativeHandler::SetPrivate(v8::Local<v8::Object> obj, const char* key, v8::Local<v8::Value> value) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << "(" << ")";
  SetPrivate(context_->v8_context(), obj, key, value);
}

// static
void ObjectBackedNativeHandler::SetPrivate(v8::Local<v8::Context> context,
                                           v8::Local<v8::Object> obj,
                                           const char* key,
                                           v8::Local<v8::Value> value) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << "(" << ")";
  obj->SetPrivate(context,
                  v8::Private::ForApi(context->GetIsolate(),
                                      v8::String::NewFromUtf8(context->GetIsolate(), key)),
                  value)
      .FromJust();
}

bool ObjectBackedNativeHandler::GetPrivate(v8::Local<v8::Object> obj, const char* key, v8::Local<v8::Value>* result) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << "(" << ")";
  return GetPrivate(context_->v8_context(), obj, key, result);
}

// static
bool ObjectBackedNativeHandler::GetPrivate(v8::Local<v8::Context> context,
                                           v8::Local<v8::Object> obj,
                                           const char* key,
                                           v8::Local<v8::Value>* result) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << "(" << ")";
  return obj->GetPrivate(context,
                         v8::Private::ForApi(
                             context->GetIsolate(), v8::String::NewFromUtf8(context->GetIsolate(), key)))
      .ToLocal(result);
}

void ObjectBackedNativeHandler::DeletePrivate(v8::Local<v8::Object> obj, const char* key) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << ")";
  DeletePrivate(context_->v8_context(), obj, key);
}

// static
void ObjectBackedNativeHandler::DeletePrivate(v8::Local<v8::Context> context, v8::Local<v8::Object> obj, const char* key) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << ")";
  obj->DeletePrivate(context,
                     v8::Private::ForApi(
                         context->GetIsolate(),
                         v8::String::NewFromUtf8(context->GetIsolate(), key)))
      .FromJust();
}

}  // namespace extensions
