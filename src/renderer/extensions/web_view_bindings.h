// Copyright (c) 2014 Stanislas Polu.
// See the LICENSE file.

#ifndef THRUST_SHELL_RENDERER_EXTENSIONS_WEB_VIEW_BINDINGS_H_
#define THRUST_SHELL_RENDERER_EXTENSIONS_WEB_VIEW_BINDINGS_H_

#include "base/values.h"

#include "renderer/extensions/object_backed_native_handler.h"
#include "renderer/guest_view_container.h"

namespace meson {
class MesonRenderFrameObserver;
}

namespace extensions {

class ScriptContext;

class WebViewBindings : public ObjectBackedNativeHandler {
 public:
  // ### WebViewBindings
  WebViewBindings(ScriptContext* context);
  ~WebViewBindings();

  // ### AttemptEmitEvent
  //
  // Attempts to emit an event for the given guest_instance_id. The event gets
  // emitted only if this WebViewBindings has an handler for it
  bool AttemptEmitEvent(int guest_instance_id, const std::string type, const base::DictionaryValue& event);

 private:
  // ### [RouteFunction]
  //
  // ```
  // @args {FunctionCallbackInfo} v8 args and return
  // ```
  void CreateGuest(const v8::FunctionCallbackInfo<v8::Value>& args);
  void AttachWindowGuest(const v8::FunctionCallbackInfo<v8::Value>& args);
  void DestroyGuest(const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetEventHandler(const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetAutoSize(const v8::FunctionCallbackInfo<v8::Value>& args);
  void Go(const v8::FunctionCallbackInfo<v8::Value>& args);
  void LoadUrl(const v8::FunctionCallbackInfo<v8::Value>& args);
  void Reload(const v8::FunctionCallbackInfo<v8::Value>& args);
  void Stop(const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetZoom(const v8::FunctionCallbackInfo<v8::Value>& args);
  void Find(const v8::FunctionCallbackInfo<v8::Value>& args);
  void StopFinding(const v8::FunctionCallbackInfo<v8::Value>& args);
  void InsertCSS(const v8::FunctionCallbackInfo<v8::Value>& args);
  void ExecuteScript(const v8::FunctionCallbackInfo<v8::Value>& args);
  void OpenDevTools(const v8::FunctionCallbackInfo<v8::Value>& args);
  void CloseDevTools(const v8::FunctionCallbackInfo<v8::Value>& args);
  void IsDevToolsOpened(const v8::FunctionCallbackInfo<v8::Value>& args);
  void JavaScriptDialogClosed(const v8::FunctionCallbackInfo<v8::Value>& args);
  void RegisterElementReiszeCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
  void OnResizeCallback(const gfx::Size& size);

  std::map<int, v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>> guest_handlers_;
  meson::MesonRenderFrameObserver* render_frame_observer_;
  v8::Persistent<v8::Function> resize_callback_;
};

}  // namespace extensions

#endif  // THRUST_SHELL_RENDERER_EXTENSIONS_WEB_VIEW_BINDINGS_H_
