// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/extensions/script_context.h"

#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"

#include "content/public/common/url_constants.h"
#include "content/public/renderer/render_view.h"
//#include "content/public/renderer/v8_value_converter.h"

#include "third_party/WebKit/public/web/WebDataSource.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
//#include "third_party/WebKit/public/web/WebScopedMicrotaskSuppression.h"
//#include "third_party/WebKit/public/web/WebSecurityOrigin.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "v8/include/v8.h"

#include "renderer/extensions/v8_helpers.h"

//using content::V8ValueConverter;

namespace extensions {

std::vector<ScriptContext*> ScriptContext::s_instances;

ScriptContext::ScriptContext(const v8::Local<v8::Context>& v8_context, blink::WebFrame* web_frame)
    : is_valid_(true),
      v8_context_(v8_context->GetIsolate(), v8_context),
      web_frame_(web_frame),
      safe_builtins_(this),
      isolate_(v8_context->GetIsolate()) {
  VLOG(1) << "Created context:\n"
          << "  frame:        " << web_frame_;
  s_instances.push_back(this);
}

ScriptContext::~ScriptContext() {
  VLOG(1) << "Destroyed context for extension";
  for (size_t i = 0; i < s_instances.size(); ++i) {
    if (s_instances[i] == this) {
      s_instances.erase(s_instances.begin() + i);
      break;
    }
  }
  Invalidate();
}

// static
ScriptContext*
ScriptContext::FromV8Context(
    const v8::Handle<v8::Context>& v8_context) {
  for (size_t i = 0; i < s_instances.size(); ++i) {
    if (s_instances[i]->v8_context() == v8_context) {
      return s_instances[i];
    }
  }
  return NULL;
}

void ScriptContext::Invalidate() {
  DCHECK(thread_checker_.CalledOnValidThread());
  CHECK(is_valid_);
  is_valid_ = false;

  // TODO(kalman): Make ModuleSystem use AddInvalidationObserver.
  // Ownership graph is a bit weird here.
  if (module_system_)
    module_system_->Invalidate();

  //TODO: implement invalidate_observers_?

  web_frame_ = nullptr;
  v8_context_.Reset();
}

content::RenderView* ScriptContext::GetRenderView() const {
  if (web_frame_ && web_frame_->view())
    return content::RenderView::FromWebView(web_frame_->view());
  else
    return NULL;
}

v8::Local<v8::Value> ScriptContext::CallFunction(v8::Handle<v8::Function> function, int argc, v8::Handle<v8::Value> argv[]) const {
  v8::EscapableHandleScope handle_scope(isolate());
  v8::Context::Scope scope(v8_context());

  v8::MicrotasksScope microtasks(isolate(), v8::MicrotasksScope::kDoNotRunMicrotasks);
  if (!is_valid()) {
    return handle_scope.Escape(v8::Local<v8::Primitive>(v8::Undefined(isolate())));
  }

  v8::Handle<v8::Object> global = v8_context()->Global();
  if (!web_frame_)
    return handle_scope.Escape(function->Call(global, argc, argv));

  return handle_scope.Escape(v8::Local<v8::Value>(web_frame_->callFunctionEvenIfScriptDisabled(function, global, argc, argv)));
}

void ScriptContext::DispatchOnUnloadEvent() {
  module_system_->CallModuleMethod("unload_event", "dispatch");
}

GURL ScriptContext::GetURL() const {
  return web_frame() ? GetDataSourceURLForFrame(web_frame()) : GURL();
}

// static
GURL ScriptContext::GetDataSourceURLForFrame(const blink::WebFrame* frame) {
  // Normally we would use frame->document().url() to determine the document's
  // URL, but to decide whether to inject a content script, we use the URL from
  // the data source. This "quirk" helps prevents content scripts from
  // inadvertently adding DOM elements to the compose iframe in Gmail because
  // the compose iframe's dataSource URL is about:blank, but the document URL
  // changes to match the parent document after Gmail document.writes into
  // it to create the editor.
  // http://code.google.com/p/chromium/issues/detail?id=86742
  blink::WebDataSource* data_source = frame->provisionalDataSource()
                                          ? frame->provisionalDataSource()
                                          : frame->dataSource();
  CHECK(data_source);
  return GURL(data_source->request().url());
}

// static
GURL ScriptContext::GetEffectiveDocumentURL(const blink::WebFrame* frame,
                                            const GURL& document_url,
                                            bool match_about_blank) {
  // Common scenario. If |match_about_blank| is false (as is the case in most
  // extensions), or if the frame is not an about:-page, just return
  // |document_url| (supposedly the URL of the frame).
  if (!match_about_blank || !document_url.SchemeIs(url::kAboutScheme))
    return document_url;

  // Non-sandboxed about:blank and about:srcdoc pages inherit their security
  // origin from their parent frame/window. So, traverse the frame/window
  // hierarchy to find the closest non-about:-page and return its URL.
  const blink::WebFrame* parent = frame;
  do {
    parent = parent->parent() ? parent->parent() : parent->opener();
  } while (parent != NULL &&
           GURL(parent->document().url()).SchemeIs(url::kAboutScheme));

  if (parent && !parent->document().isNull()) {
    // Only return the parent URL if the frame can access it.
    const blink::WebDocument& parent_document = parent->document();
    if (frame->document().getSecurityOrigin().canAccess(
            parent_document.getSecurityOrigin()))
      return parent_document.url();
  }
  return document_url;
}

v8::Local<v8::Value> ScriptContext::RunScript(v8::Local<v8::String> name,
                                              v8::Local<v8::String> code,
                                              const RunScriptExceptionHandler& exception_handler) {
  DCHECK(thread_checker_.CalledOnValidThread());
  v8::EscapableHandleScope handle_scope(isolate());
  v8::Context::Scope context_scope(v8_context());

  // Prepend extensions:: to |name| so that internal code can be differentiated
  // from external code in stack traces. This has no effect on behaviour.
  std::string internal_name = base::StringPrintf("extensions::%s", *v8::String::Utf8Value(name));

  if (internal_name.size() >= v8::String::kMaxLength) {
    NOTREACHED() << "internal_name is too long.";
    return v8::Undefined(isolate());
  }

  v8::MicrotasksScope microtasks(isolate(), v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::TryCatch try_catch(isolate());
  try_catch.SetCaptureMessage(true);
  v8::ScriptOrigin origin(v8_helpers::ToV8StringUnsafe(isolate(), internal_name.c_str()));
  v8::Local<v8::Script> script;
  if (!v8::Script::Compile(v8_context(), code, &origin).ToLocal(&script)) {
    exception_handler.Run(try_catch);
    return v8::Undefined(isolate());
  }

  v8::Local<v8::Value> result;
  if (!script->Run(v8_context()).ToLocal(&result)) {
    exception_handler.Run(try_catch);
    return v8::Undefined(isolate());
  }

  return handle_scope.Escape(result);
}

}  // namespace extensions
