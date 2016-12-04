#include "content/public/app/content_main.h"
#include "app/main_delegate.h"
#include "common/meson_command_line.h"

namespace meson {
int MesonMain(int argc, const char* argv[]) {
  MesonCommandLine::Init(argc, argv);
  meson::MainDelegate delegate;
  content::ContentMainParams params(&delegate);
  params.argc = argc;
  params.argv = argv;
  return content::ContentMain(params);
}
}
