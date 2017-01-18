// Copyright (c) 2014 Stanislas Polu.
// Copyright (c) 2013 The Chromium Authors.
// See the LICENSE file.

#include "renderer/extensions/document_bindings.h"

#include <string>

#include "base/bind.h"
#include "base/values.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "v8/include/v8.h"
#include "content/public/child/v8_value_converter.h"

#include "renderer/extensions/script_context.h"

namespace extensions {

DocumentBindings::DocumentBindings(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {
  RouteFunction("RegisterElement", base::Bind(&DocumentBindings::RegisterElement, base::Unretained(this)));
  RouteFunction("GetZoomFactor", base::Bind(&DocumentBindings::GetZoomFactor, base::Unretained(this)));
}

void DocumentBindings::RegisterElement(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsObject()) {
    NOTREACHED();
    return;
  }

  std::string element_name(*v8::String::Utf8Value(args[0]));
  v8::Local<v8::Object> options = args[1]->ToObject();
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << element_name << ", "
            << "?"
            << ")";

  blink::WebExceptionCode ec = 0;
  blink::WebDocument document = context()->web_frame()->document();

  v8::Handle<v8::Value> constructor = document.registerEmbedderCustomElement(blink::WebString::fromUTF8(element_name), options, ec);
  args.GetReturnValue().Set(constructor);
}


void DocumentBindings::GetZoomFactor(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << getZoomLevel();
  if (args.Length() != 0) {
    NOTREACHED();
    return;
  }
  double zoomFactor = blink::WebView::zoomLevelToZoomFactor(getZoomLevel());
  args.GetReturnValue().Set(v8::Number::New(context()->isolate(), zoomFactor));
}

double DocumentBindings::getZoomLevel(void) const {
  return context()->web_frame()->view()->zoomLevel();
}


}  // namespace extensions
