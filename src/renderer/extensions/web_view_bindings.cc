// Copyright (c) 2014 Stanislas Polu.
// See the LICENSE file.

#include "renderer/extensions/web_view_bindings.h"

#include <string>

#include "base/bind.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "v8/include/v8.h"
#include "content/public/child/v8_value_converter.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

#include "renderer/extensions/script_context.h"
#include "api/api_messages.h"
#include "renderer/meson_render_frame_observer.h"

using namespace content;

namespace extensions {

WebViewBindings::WebViewBindings(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  RouteFunction("CreateGuest",
                base::Bind(&WebViewBindings::CreateGuest,
                           base::Unretained(this)));
  RouteFunction("AttachWindowGuest",
                base::Bind(&WebViewBindings::AttachWindowGuest,
                           base::Unretained(this)));
  RouteFunction("DestroyGuest",
                base::Bind(&WebViewBindings::DestroyGuest,
                           base::Unretained(this)));
  RouteFunction("SetEventHandler",
                base::Bind(&WebViewBindings::SetEventHandler,
                           base::Unretained(this)));
  RouteFunction("SetAutoSize",
                base::Bind(&WebViewBindings::SetAutoSize,
                           base::Unretained(this)));
  RouteFunction("Go",
                base::Bind(&WebViewBindings::Go,
                           base::Unretained(this)));
  RouteFunction("LoadUrl",
                base::Bind(&WebViewBindings::LoadUrl,
                           base::Unretained(this)));
  RouteFunction("Reload",
                base::Bind(&WebViewBindings::Reload,
                           base::Unretained(this)));
  RouteFunction("Stop",
                base::Bind(&WebViewBindings::Stop,
                           base::Unretained(this)));
  RouteFunction("SetZoom",
                base::Bind(&WebViewBindings::SetZoom,
                           base::Unretained(this)));
  RouteFunction("Find",
                base::Bind(&WebViewBindings::Find,
                           base::Unretained(this)));
  RouteFunction("StopFinding",
                base::Bind(&WebViewBindings::StopFinding,
                           base::Unretained(this)));
  RouteFunction("InsertCSS",
                base::Bind(&WebViewBindings::InsertCSS,
                           base::Unretained(this)));
  RouteFunction("ExecuteScript",
                base::Bind(&WebViewBindings::ExecuteScript,
                           base::Unretained(this)));
  RouteFunction("OpenDevTools",
                base::Bind(&WebViewBindings::OpenDevTools,
                           base::Unretained(this)));
  RouteFunction("CloseDevTools",
                base::Bind(&WebViewBindings::CloseDevTools,
                           base::Unretained(this)));
  RouteFunction("IsDevToolsOpened",
                base::Bind(&WebViewBindings::IsDevToolsOpened,
                           base::Unretained(this)));
  RouteFunction("JavaScriptDialogClosed",
                base::Bind(&WebViewBindings::JavaScriptDialogClosed,
                           base::Unretained(this)));

  RouteFunction("RegisterElementResizeCallback", base::Bind(&WebViewBindings::RegisterElementReiszeCallback, base::Unretained(this)));

  render_frame_observer_ = meson::MesonRenderFrameObserver::FromRenderFrame(
      RenderFrame::FromWebFrame(this->context()->web_frame()));
  render_frame_observer_->AddWebViewBindings(this);

  LOG(INFO) << "WebViewBindings Constructor " << this;
}

WebViewBindings::~WebViewBindings() {
  render_frame_observer_->RemoveWebViewBindings(this);

  LOG(INFO) << "WebViewBindings Destructor " << this;
}

bool WebViewBindings::AttemptEmitEvent(int guest_instance_id,
                                       const std::string type,
                                       const base::DictionaryValue& event) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << guest_instance_id << ", " << type << ", " << event << ")";
  if (guest_handlers_.find(guest_instance_id) != guest_handlers_.end()) {
    v8::HandleScope handle_scope(context()->isolate());

    v8::Local<v8::String> type_arg = v8::String::NewFromUtf8(context()->isolate(), type.c_str());
    std::unique_ptr<V8ValueConverter> converter(V8ValueConverter::create());
    v8::Handle<v8::Value> event_arg = converter->ToV8Value(&event, context()->v8_context());
    v8::Local<v8::Function> handler = v8::Local<v8::Function>::New(context()->isolate(),
                                                                   guest_handlers_[guest_instance_id]);

    v8::Local<v8::Value> argv[2] = {type_arg, event_arg};
    context()->CallFunction(handler, 2, argv);
  }
  return false;
}

void WebViewBindings::CreateGuest(const v8::FunctionCallbackInfo<v8::Value>& args) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << "(" << args.Length() << ")";
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

  std::unique_ptr<base::DictionaryValue> params(static_cast<base::DictionaryValue*>(value.release()));

  LOG(INFO) << "WEB_VIEW_BINDINGS: CreateGuest";

  int id = 0;
  render_frame_observer_->Send(new MesonFrameHostMsg_CreateWebViewGuest(
      render_frame_observer_->routing_id(),
      *params.get(), &id));

  args.GetReturnValue().Set(v8::Integer::New(context()->isolate(), id));
}

void WebViewBindings::AttachWindowGuest(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << args.Length() << ")";
  if (args.Length() != 3 || !args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsObject()) {
    NOTREACHED();
    return;
  }

  int internal_instance_id = args[0]->NumberValue();
  int guest_instance_id = args[1]->NumberValue();
  v8::Local<v8::Object> object = args[2]->ToObject();
  std::unique_ptr<V8ValueConverter> converter(V8ValueConverter::create());
  std::unique_ptr<base::Value> value(converter->FromV8Value(object, context()->v8_context()));

  if (!value) {
    return;
  }
  if (!value->IsType(base::Value::TYPE_DICTIONARY)) {
    return;
  }
  std::unique_ptr<base::DictionaryValue> params(static_cast<base::DictionaryValue*>(value.release()));

  LOG(INFO) << "WEB_VIEW_BINDINGS: AttachWindowGuest " << guest_instance_id;

  render_frame_observer_->Send(new MesonFrameHostMsg_AttachWindowGuest(
      render_frame_observer_->routing_id(),
      internal_instance_id,
      guest_instance_id,
      *params.get()));

  content::RenderFrame::FromWebFrame(blink::WebLocalFrame::frameForCurrentContext())->AttachGuest(internal_instance_id);
}

void WebViewBindings::DestroyGuest(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  LOG(INFO) << "WEB_VIEW_BINDINGS: DestroyGuest " << guest_instance_id;

  render_frame_observer_->Send(new MesonFrameHostMsg_DestroyWebViewGuest(
      render_frame_observer_->routing_id(),
      guest_instance_id));
}

void WebViewBindings::SetEventHandler(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsFunction()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  LOG(INFO) << "WEB_VIEW_BINDINGS: SetEventHandler " << guest_instance_id;

  v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(args[1]);
  guest_handlers_[guest_instance_id].Reset(context()->isolate(), cb);
}

void WebViewBindings::SetAutoSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();

  if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsObject()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  v8::Local<v8::Object> object = args[1]->ToObject();

  std::unique_ptr<V8ValueConverter> converter(V8ValueConverter::create());
  std::unique_ptr<base::Value> value(converter->FromV8Value(object, context()->v8_context()));

  if (!value) {
    return;
  }
  if (!value->IsType(base::Value::TYPE_DICTIONARY)) {
    return;
  }

  std::unique_ptr<base::DictionaryValue> params(static_cast<base::DictionaryValue*>(value.release()));

  LOG(INFO) << "WEB_VIEW_BINDINGS: SetAutoSize " << guest_instance_id;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestSetAutoSize(render_frame_observer_->routing_id(),
                                                                             guest_instance_id,
                                                                             *params.get()));
}

void WebViewBindings::Go(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsNumber()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  int index = args[1]->NumberValue();

  LOG(INFO) << "WEB_VIEW_BINDINGS: Go " << guest_instance_id << " " << index;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestGo(
      render_frame_observer_->routing_id(),
      guest_instance_id, index));
}

void WebViewBindings::LoadUrl(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsString()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  std::string url(*v8::String::Utf8Value(args[1]));

  LOG(INFO) << "WEB_VIEW_BINDINGS: LoadUrl " << guest_instance_id << " " << url;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestLoadUrl(
      render_frame_observer_->routing_id(),
      guest_instance_id, url));
}

void WebViewBindings::Reload(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsBoolean()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  bool ignore_cache = args[1]->BooleanValue();

  LOG(INFO) << "WEB_VIEW_BINDINGS: Reload " << guest_instance_id << " " << ignore_cache;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestReload(
      render_frame_observer_->routing_id(),
      guest_instance_id, ignore_cache));
}

void WebViewBindings::Stop(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();

  LOG(INFO) << "WEB_VIEW_BINDINGS: Stop " << guest_instance_id;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestStop(
      render_frame_observer_->routing_id(),
      guest_instance_id));
}

void WebViewBindings::SetZoom(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsNumber()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  double zoom_factor = args[1]->NumberValue();

  LOG(INFO) << "WEB_VIEW_BINDINGS: SetZoom " << guest_instance_id << " " << zoom_factor;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestSetZoom(
      render_frame_observer_->routing_id(),
      guest_instance_id, zoom_factor));
}

void WebViewBindings::Find(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 4 || !args[0]->IsNumber() ||
      !args[1]->IsNumber() || !args[2]->IsString() || !args[3]->IsObject()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  int request_id = args[1]->NumberValue();
  std::string search_text(*v8::String::Utf8Value(args[2]));
  v8::Local<v8::Object> object = args[3]->ToObject();

  std::unique_ptr<V8ValueConverter> converter(V8ValueConverter::create());
  std::unique_ptr<base::Value> value(converter->FromV8Value(object, context()->v8_context()));

  if (!value) {
    return;
  }
  if (!value->IsType(base::Value::TYPE_DICTIONARY)) {
    return;
  }

  std::unique_ptr<base::DictionaryValue> options(static_cast<base::DictionaryValue*>(value.release()));

  LOG(INFO) << "WEB_VIEW_BINDINGS: Find " << guest_instance_id << " "
            << request_id << " " << search_text;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestFind(
      render_frame_observer_->routing_id(),
      guest_instance_id, request_id, search_text, *options.get()));
}

void WebViewBindings::StopFinding(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsString()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  std::string action(*v8::String::Utf8Value(args[1]));

  LOG(INFO) << "WEB_VIEW_BINDINGS: StopFinding " << guest_instance_id << " " << action;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestStopFinding(
      render_frame_observer_->routing_id(),
      guest_instance_id, action));
}

void WebViewBindings::InsertCSS(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsString()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  std::string css(*v8::String::Utf8Value(args[1]));

  LOG(INFO) << "WEB_VIEW_BINDINGS: InsertCSS " << guest_instance_id;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestInsertCSS(
      render_frame_observer_->routing_id(),
      guest_instance_id, css));
}

void WebViewBindings::ExecuteScript(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsString()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  std::string script(*v8::String::Utf8Value(args[1]));

  LOG(INFO) << "WEB_VIEW_BINDINGS: ExecuteScript " << guest_instance_id;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestExecuteScript(
      render_frame_observer_->routing_id(),
      guest_instance_id, script));
}

void WebViewBindings::OpenDevTools(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();

  LOG(INFO) << "WEB_VIEW_BINDINGS: OpenDevTools " << guest_instance_id;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestOpenDevTools(
      render_frame_observer_->routing_id(),
      guest_instance_id));
}

void WebViewBindings::CloseDevTools(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();

  LOG(INFO) << "WEB_VIEW_BINDINGS: CloseDevTools " << guest_instance_id;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestCloseDevTools(
      render_frame_observer_->routing_id(),
      guest_instance_id));
}

void WebViewBindings::IsDevToolsOpened(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();

  LOG(INFO) << "WEB_VIEW_BINDINGS: IsDevToolsOpened " << guest_instance_id;

  bool open = false;
  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestIsDevToolsOpened(
      render_frame_observer_->routing_id(),
      guest_instance_id, &open));

  args.GetReturnValue().Set(v8::Boolean::New(context()->isolate(), open));
}

void WebViewBindings::JavaScriptDialogClosed(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " : " << args.Length();
  if (args.Length() != 3 || !args[0]->IsNumber() ||
      !args[1]->IsBoolean() || !args[2]->IsString()) {
    NOTREACHED();
    return;
  }

  int guest_instance_id = args[0]->NumberValue();
  bool success = args[1]->BooleanValue();
  std::string response(*v8::String::Utf8Value(args[2]));

  LOG(INFO) << "WEB_VIEW_BINDINGS: JavaScriptDialogClosed " << guest_instance_id << " "
            << success << " " << response;

  render_frame_observer_->Send(new MesonFrameHostMsg_WebViewGuestJavaScriptDialogClosed(
      render_frame_observer_->routing_id(),
      guest_instance_id, success, response));
}

void WebViewBindings::RegisterElementReiszeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsFunction()) {
    NOTREACHED();
    return;
  }
  int element_instance_id = args[0]->NumberValue();

  auto guest_view_container = meson::GuestViewContainer::FromID(element_instance_id);
  if (guest_view_container) {
    auto cb = v8::Local<v8::Function>::Cast(args[1]);
    resize_callback_.Reset(context()->isolate(), cb);
    auto p = base::Bind(&WebViewBindings::OnResizeCallback, base::Unretained(this));
    guest_view_container->RegisterElementResizeCallback(p);
  }
}

void WebViewBindings::OnResizeCallback(const gfx::Size& size) {
  LOG(INFO) << __PRETTY_FUNCTION__ << "(" << size.width() << ", " << size.height() << ")";
  if (!resize_callback_.IsEmpty()) {
    v8::HandleScope handle_scope(context()->isolate());
    base::DictionaryValue event;
    event.SetInteger("width", size.width());
    event.SetInteger("height", size.height());
    std::unique_ptr<content::V8ValueConverter> converter(content::V8ValueConverter::create());
    v8::Handle<v8::Value> event_arg(converter->ToV8Value(&event, context()->v8_context()));
    auto handler = v8::Local<v8::Function>::New(context()->isolate(), resize_callback_);
    v8::Local<v8::Value> argv[1] = {event_arg};
    context()->CallFunction(handler, 1, argv);
  }
}

}  // namespace extensions
