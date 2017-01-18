#include "renderer/meson_render_view_observer.h"

#include <string>
#include <vector>

// Put this before event_emitter_caller.h to have string16 support.
#include "api/api_messages.h"
//#include "common/api/event_emitter_caller.h"
#include "common/options_switches.h"
#include "renderer/renderer_client.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message_macros.h"
#include "net/base/net_module.h"
#include "net/grit/net_resources.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebDraggableRegion.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "ui/base/resource/resource_bundle.h"

namespace meson {

namespace {

#if 0
bool GetIPCObject(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  v8::Local<v8::Object>* ipc) {
  v8::Local<v8::String> key = mate::StringToV8(isolate, "ipc");
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
  v8::Local<v8::Object> global_object = context->Global();
  v8::Local<v8::Value> value;
  if (!global_object->GetPrivate(context, privateKey).ToLocal(&value))
    return false;
  if (value.IsEmpty() || !value->IsObject())
    return false;
  *ipc = value->ToObject();
  return true;
}


std::vector<v8::Local<v8::Value>> ListValueToVector(
    v8::Isolate* isolate,
    const base::ListValue& list) {
  v8::Local<v8::Value> array = mate::ConvertToV8(isolate, list);
  std::vector<v8::Local<v8::Value>> result;
  mate::ConvertFromV8(isolate, array, &result);
  return result;
}
#endif

base::StringPiece NetResourceProvider(int key) {
  if (key == IDR_DIR_HEADER_HTML) {
    base::StringPiece html_data = ui::ResourceBundle::GetSharedInstance().GetRawDataResource(IDR_DIR_HEADER_HTML);
    return html_data;
  }
  return base::StringPiece();
}

}  // namespace

MesonRenderViewObserver::MesonRenderViewObserver(content::RenderView* render_view, MesonRendererClient* renderer_client)
    : content::RenderViewObserver(render_view), document_created_(false) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  // Initialise resource for directory listing.
  net::NetModule::SetResourceProvider(NetResourceProvider);
}

MesonRenderViewObserver::~MesonRenderViewObserver() {}

void MesonRenderViewObserver::EmitIPCEvent(blink::WebFrame* frame, const base::string16& channel, const base::ListValue& args) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (!frame || frame->isWebRemoteFrame())
    return;

  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);

#if 0
  // Only emit IPC event for context with node integration.
  node::Environment* env = node::Environment::GetCurrent(context);
  if (!env)
    return;

  v8::Local<v8::Object> ipc;
  if (GetIPCObject(isolate, context, &ipc)) {
    auto args_vector = ListValueToVector(isolate, args);
    // Insert the Event object, event.sender is ipc.
    mate::Dictionary event = mate::Dictionary::CreateEmpty(isolate);
    event.Set("sender", ipc);
    args_vector.insert(args_vector.begin(), event.GetHandle());
    mate::EmitEvent(isolate, ipc, channel, args_vector);
  }
#endif
}

void MesonRenderViewObserver::DidCreateDocumentElement(blink::WebLocalFrame* frame) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  document_created_ = true;

  // Read --zoom-factor from command line.
  std::string zoom_factor_str = base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(switches::kZoomFactor);
  if (zoom_factor_str.empty())
    return;
  double zoom_factor;
  if (!base::StringToDouble(zoom_factor_str, &zoom_factor))
    return;
  double zoom_level = blink::WebView::zoomFactorToZoomLevel(zoom_factor);
  frame->view()->setZoomLevel(zoom_level);
}

void MesonRenderViewObserver::DraggableRegionsChanged(blink::WebFrame* frame) {
  blink::WebVector<blink::WebDraggableRegion> webregions = frame->document().draggableRegions();
  std::vector<DraggableRegion> regions;
  for (auto& webregion : webregions) {
    DraggableRegion region;
    render_view()->ConvertViewportToWindowViaWidget(&webregion.bounds);
    region.bounds = webregion.bounds;
    region.draggable = webregion.draggable;
    regions.push_back(region);
  }
  Send(new MesonViewHostMsg_UpdateDraggableRegions(routing_id(), regions));
}

bool MesonRenderViewObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MesonRenderViewObserver, message)
    IPC_MESSAGE_HANDLER(MesonViewMsg_Message, OnBrowserMessage)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void MesonRenderViewObserver::OnDestruct() {
  LOG(INFO) << __PRETTY_FUNCTION__ << (long )this;
  delete this;
}

void MesonRenderViewObserver::OnBrowserMessage(bool send_to_all, const base::string16& channel, const base::ListValue& args) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  if (!document_created_)
    return;

  if (!render_view()->GetWebView())
    return;

  blink::WebFrame* frame = render_view()->GetWebView()->mainFrame();
  if (!frame || frame->isWebRemoteFrame())
    return;

  EmitIPCEvent(frame, channel, args);

  // Also send the message to all sub-frames.
  if (send_to_all) {
    for (blink::WebFrame* child = frame->firstChild(); child; child = child->nextSibling())
      EmitIPCEvent(child, channel, args);
  }
}
}
