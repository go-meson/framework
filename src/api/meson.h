//-*-c++-*-
#ifndef __INC_MESON_API__
#define __INC_MESON_API__

#ifdef __cplusplus
extern "C" {
#endif

#define MESON_EXPORT __attribute__((visibility("default")))

typedef enum MESON_ACTION_TYPE {
  MESON_ACTION_TYPE_CREATE = 0,
  MESON_ACTION_TYPE_DELETE,
  MESON_ACTION_TYPE_CALL,
  MESON_ACTION_TYPE_REPLY,
  MESON_ACTION_TYPE_EVENT,
  MESON_ACTION_TYPE_REGISTER_EVENT,
} MESON_ACTION_TYPE;

// singleton object id
enum {
  MESON_OBJID_STATIC = 0,
};

// object types
typedef enum MESON_OBJECT_TYPE {
  MESON_OBJECT_TYPE_NULL = 0,
  MESON_OBJECT_TYPE_APP,
  MESON_OBJECT_TYPE_WINDOW,
  MESON_OBJECT_TYPE_SESSION,
  MESON_OBJECT_TYPE_WEB_CONTENTS,
  MESON_OBJECT_TYPE_MENU,
  MESON_OBJECT_TYPE_DIALOG,

  MESON_OBJECT_TYPE_NUM
} MESON_OBJECT_TYPE;

/*------------------------------------------------------------------------
 * menu definition
 */
typedef enum MESON_MENU_TYPE {
  MESON_MENU_TYPE_NORMAL = 0,
  MESON_MENU_TYPE_SEPARATOR,
  MESON_MENU_TYPE_SUBMENU,
  MESON_MENU_TYPE_CHECKBOX,
  MESON_MENU_TYPE_RADIO,
} MESON_MENU_TYPE;

/*------------------------------------------------------------------------
 * dialog definition
 */
typedef enum MESON_DIALOG_MESSAGEBOX_TYPES {
  MESON_DIALOG_MESSAGEBOX_TYPE_NONE = 0,
  MESON_DIALOG_MESSAGEBOX_TYPE_INFO,
  MESON_DIALOG_MESSAGEBOX_TYPE_WARNING,
  MESON_DIALOG_MESSAGEBOX_TYPE_ERROR,
  MESON_DIALOG_MESSAGEBOX_TYPE_QUESTION,
} MESON_DIALOG_MESSAGEBOX_TYPES;

typedef enum MESON_DIALOG_OPTIONS_TYPE {
  MESON_DIALOG_OPTIONS_TYPE_NONE = 0,
  MESON_DIALOG_OPTIONS_TYPE_NOLINK = (1 << 0),
} MESON_DIALOG_OPTIONS_TYPE;

/*------------------------------------------------------------------------
 * export functions
 */
MESON_EXPORT void MesonApiSetArgc(int argc);
MESON_EXPORT void MesonApiAddArgv(int i, const char* argv);
MESON_EXPORT int MesonApiMain(void);

typedef void (*MesonInitHandler)(void);
typedef char* (*MesonWaitServerRequestHandler)(void);
typedef char* (*MesonPostServerResponseHandler)(unsigned int id, const char* pMsg, int needReply);

MESON_EXPORT void MesonApiSetHandler(MesonInitHandler pfnInitHandler,
                                     MesonWaitServerRequestHandler pfnWaitHandler,
                                     MesonPostServerResponseHandler pfnPostHandler);

/*------------------------------------------------------------------------
 * internal functions
 */
#ifdef __cplusplus
bool mesonApiCheckInitHandler(void);
void mesonApiCallInitHandler(void);
char* mesonApiCallWaitServerRequestHandler(void);
char* mesonApiPostServerResponseHandler(unsigned int id, const char* msg, bool needReply = false);
}
#endif

#endif
