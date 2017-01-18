//-*-c++-+-

// Multiply-included file, no traditional include guard.

#include "common/draggable_region.h"
#include "base/strings/string16.h"
#include "base/values.h"
#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/ipc/gfx_param_traits.h"

// The message starter should be declared in ipc/ipc_message_start.h. Since
// we don't want to patch Chromium, we just pretend to be Content Shell.

#define IPC_MESSAGE_START ShellMsgStart

IPC_STRUCT_TRAITS_BEGIN(meson::DraggableRegion)
  IPC_STRUCT_TRAITS_MEMBER(draggable)
  IPC_STRUCT_TRAITS_MEMBER(bounds)
IPC_STRUCT_TRAITS_END()

IPC_MESSAGE_ROUTED2(MesonViewHostMsg_Message,
                    base::string16 /* channel */,
                    base::ListValue /* arguments */)

IPC_SYNC_MESSAGE_ROUTED2_1(MesonViewHostMsg_Message_Sync,
                           base::string16 /* channel */,
                           base::ListValue /* arguments */,
                           base::string16 /* result (in JSON) */)

IPC_MESSAGE_ROUTED3(MesonViewMsg_Message,
                    bool /* send_to_all */,
                    base::string16 /* channel */,
                    base::ListValue /* arguments */)

// Sent by the renderer when the draggable regions are updated.
IPC_MESSAGE_ROUTED1(MesonViewHostMsg_UpdateDraggableRegions,
                    std::vector<meson::DraggableRegion> /* regions */)

///
// CreateWebViewGuest
IPC_SYNC_MESSAGE_ROUTED1_1(MesonFrameHostMsg_CreateWebViewGuest,
                           base::DictionaryValue, /* params */
                           int /* guest_instance_id */)

// AttachWindowGuest
IPC_MESSAGE_ROUTED3(MesonFrameHostMsg_AttachWindowGuest,
                    int, /*internal_instance_id*/
                    int, /*guest_instance_id*/
                    base::DictionaryValue /* params */)

// DestroyWebViewGuest
IPC_MESSAGE_ROUTED1(MesonFrameHostMsg_DestroyWebViewGuest,
                    int /* guest_instance_id */)
// WebViewSetAutoSize
IPC_MESSAGE_ROUTED2(MesonFrameHostMsg_WebViewGuestSetAutoSize,
                    int, /* guest_instance_id */
                    base::DictionaryValue /* params */)

// WebViewGuestGo
IPC_MESSAGE_ROUTED2(MesonFrameHostMsg_WebViewGuestGo,
                    int, /* guest_instance_id */
                    int /* relative_index */)

// WebViewGuestLoadUrl
IPC_MESSAGE_ROUTED2(MesonFrameHostMsg_WebViewGuestLoadUrl,
                    int, /* guest_instance_id */
                    std::string /* url */)

// WebViewGuestReload
IPC_MESSAGE_ROUTED2(MesonFrameHostMsg_WebViewGuestReload,
                    int, /* guest_instance_id */
                    bool /* ignore_cache */)

// WebViewGuestStop
IPC_MESSAGE_ROUTED1(MesonFrameHostMsg_WebViewGuestStop,
                    int /* guest_instance_id */)

// WebViewGuestSetZoom
IPC_MESSAGE_ROUTED2(MesonFrameHostMsg_WebViewGuestSetZoom,
                    int, /* guest_instance_id */
                    double /* zoom_factor */)

// WebViewGuestFind
IPC_MESSAGE_ROUTED4(MesonFrameHostMsg_WebViewGuestFind,
                    int,         /* guest_instance_id */
                    int,         /* request_id */
                    std::string, /* search_text */
                    base::DictionaryValue /* options */)

// WebViewGuestStopFinding
IPC_MESSAGE_ROUTED2(MesonFrameHostMsg_WebViewGuestStopFinding,
                    int, /* guest_instance_id */
                    std::string /* action */)

// WebViewGuestInsertCSS
IPC_MESSAGE_ROUTED2(MesonFrameHostMsg_WebViewGuestInsertCSS,
                    int, /* guest_instance_id */
                    std::string /* css */)

// WebViewGuestExecuteScript
IPC_MESSAGE_ROUTED2(MesonFrameHostMsg_WebViewGuestExecuteScript,
                    int, /* guest_instance_id */
                    std::string /* script */)

// WebViewGuestOpenDevTools
IPC_MESSAGE_ROUTED1(MesonFrameHostMsg_WebViewGuestOpenDevTools,
                    int /* guest_instance_id */)

// WebViewGuestCloseDevTools
IPC_MESSAGE_ROUTED1(MesonFrameHostMsg_WebViewGuestCloseDevTools,
                    int /* guest_instance_id */)

// WebViewGuestIsDevToolsOpened
IPC_SYNC_MESSAGE_ROUTED1_1(MesonFrameHostMsg_WebViewGuestIsDevToolsOpened,
                           int, /* guest_instance_id */
                           bool /* open */)

// WebViewGuestJavaScriptDialogClosed
IPC_MESSAGE_ROUTED3(MesonFrameHostMsg_WebViewGuestJavaScriptDialogClosed,
                    int,  /* guest_instance_id */
                    bool, /* succces */
                    std::string /* response */)

// WebViewEmit
IPC_MESSAGE_ROUTED3(MesonFrameMsg_WebViewEmit,
                    int,         /* guest_instance_id */
                    std::string, /* type */
                    base::DictionaryValue /* event */)

// RemoteSendMessage
IPC_MESSAGE_ROUTED1(MesonFrameHostMsg_RemoteSend,
                    base::DictionaryValue /* message */)
// RemoteDispatchMessage
IPC_MESSAGE_ROUTED1(MesonFrameMsg_RemoteDispatch,
                    base::DictionaryValue /* message */)

// Update renderer process preferences.
IPC_MESSAGE_CONTROL1(MesonMsg_UpdatePreferences, base::ListValue)
