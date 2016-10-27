#include "content/public/app/content_main.h"
#include "main_delegate.h"

namespace meson {
int MesonMain(int argc, const char* argv[])
{
  meson::MainDelegate delegate;
  content::ContentMainParams params(&delegate);
  params.argc = argc;
  params.argv = argv;
  return content::ContentMain(params);
}
}
