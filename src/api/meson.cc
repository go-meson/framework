#include "meson.h"
#include "app/library_main.h"
#include <string>
#include <vector>

namespace {
std::vector<const char*> s_args;
MesonInitHandler s_pfnInitHandler = nullptr;
MesonWaitServerRequestHandler s_pfnWaitServerRequestHandler = nullptr;
MesonPostServerResponseHandler s_pfnPostServerResponseHandler = nullptr;
}

void MesonApiSetArgc(int argc) {
  s_args.resize(argc);
}
void MesonApiAddArgv(int i, const char* argv) {
  s_args[i] = argv;
}
int MesonApiMain(void) {
  return meson::MesonMain(static_cast<int>(s_args.size()), s_args.data());
}
void MesonApiSetHandler(MesonInitHandler pfnInitHandler,
                        MesonWaitServerRequestHandler pfnWaitHandler,
                        MesonPostServerResponseHandler pfnPostHandler) {
  s_pfnInitHandler = pfnInitHandler;
  s_pfnWaitServerRequestHandler = pfnWaitHandler;
  s_pfnPostServerResponseHandler = pfnPostHandler;
}

bool mesonApiCheckInitHandler(void) {
  return s_pfnInitHandler != nullptr;
}
void mesonApiCallInitHandler(void) {
  if (s_pfnInitHandler) {
    (*s_pfnInitHandler)();
  }
}
char* mesonApiCallWaitServerRequestHandler(void) {
  if (!s_pfnWaitServerRequestHandler) {
    return nullptr;
  }
  return (*s_pfnWaitServerRequestHandler)();
}

char* mesonApiPostServerResponseHandler(unsigned int id, const char* msg, bool needReply /* = false */) {
  if (!s_pfnPostServerResponseHandler) {
    return nullptr;
  }
  return (*s_pfnPostServerResponseHandler)(id, msg, needReply ? 1 : 0);
}
