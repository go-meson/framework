// Copyright (c) 2014 Stanislas Polu.
// See the LICENSE file.

#include "renderer/extensions/remote_bindings.h"

#include <string>

#include "base/bind.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "v8/include/v8.h"
#include "content/public/child/v8_value_converter.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"

#include "renderer/extensions/script_context.h"
#include "api/api_messages.h"
#include "renderer/meson_render_frame_observer.h"

using namespace content;

namespace extensions {

RemoteBindings::RemoteBindings(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {
  RouteFunction("SendMessage",
                base::Bind(&RemoteBindings::SendMessage,
                           base::Unretained(this)));
  RouteFunction("SetHandler",
                base::Bind(&RemoteBindings::SetHandler,
                           base::Unretained(this)));

  render_frame_observer_ = meson::MesonRenderFrameObserver::FromRenderFrame(RenderFrame::FromWebFrame(this->context()->web_frame()));
  render_frame_observer_->AddRemoteBindings(this);

  LOG(INFO) << "RemoteBindings Constructor " << this;
}

RemoteBindings::~RemoteBindings() {
  render_frame_observer_->RemoveRemoteBindings(this);

  LOG(INFO) << "RemoteBindings Destructor " << this;
}

bool RemoteBindings::DispatchMessage(const base::DictionaryValue& message) {
  if (!message_handler_.IsEmpty()) {
    v8::HandleScope handle_scope(context()->isolate());

    std::unique_ptr<V8ValueConverter> converter(V8ValueConverter::create());
    v8::Handle<v8::Value> message_arg = converter->ToV8Value(&message, context()->v8_context());
    v8::Local<v8::Function> handler = v8::Local<v8::Function>::New(context()->isolate(), message_handler_);
    v8::Local<v8::Value> argv[1] = {message_arg};
    context()->CallFunction(handler, 1, argv);
  }
  return false;
}

void RemoteBindings::SendMessage(const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1 || !args[0]->IsObject()) {
    NOTREACHED();
    return;
  }

  v8::Local<v8::Object> object = args[0]->ToObject();

  std::unique_ptr<V8ValueConverter> converter(V8ValueConverter::create());
  std::unique_ptr<base::Value> value(converter->FromV8Value(object, context()->v8_context()));

  if (!value) {
    return;
  }
  if (!value->IsType(base::Value::TYPE_DICTIONARY)) {
    return;
  }

  std::unique_ptr<base::DictionaryValue> message(static_cast<base::DictionaryValue*>(value.release()));
  LOG(INFO) << "REMOTE_BINDINGS: SendMessage : " << *message;

  render_frame_observer_->Send(new MesonFrameHostMsg_RemoteSend(
      render_frame_observer_->routing_id(),
      *message.get()));
}

void RemoteBindings::SetHandler(const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1 || !args[0]->IsFunction()) {
    NOTREACHED();
    return;
  }

  LOG(INFO) << "REMOTE_BINDINGS: SetHandler";

  v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(args[0]);
  message_handler_.Reset(context()->isolate(), cb);
}

}  // extensions
