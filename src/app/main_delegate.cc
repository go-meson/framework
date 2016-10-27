#include "main_delegate.h"

#include "base/command_line.h"
#include "content/public/common/content_switches.h"

#include "browser/browser_client.h"
#include "renderer/renderer_client.h"
#include "app/content_client.h"

namespace meson {
MainDelegate::MainDelegate() {}
MainDelegate::~MainDelegate() {}

std::unique_ptr<brightray::ContentClient> MainDelegate::CreateContentClient() {
  return std::unique_ptr<meson::ContentClient>(new meson::ContentClient());
}

std::unique_ptr<brightray::BrowserClient> MainDelegate::CreateBrowserClient() {
  return std::unique_ptr<brightray::BrowserClient>(
      new meson::MesonBrowserClient);
}

bool MainDelegate::BasicStartupComplete(int* exit_code) {
  // TODO: log?
  return brightray::MainDelegate::BasicStartupComplete(exit_code);
}

void MainDelegate::PreSandboxStartup(void) {
  brightray::MainDelegate::PreSandboxStartup();
  auto command_line = base::CommandLine::ForCurrentProcess();
  auto process_type = command_line->GetSwitchValueASCII(switches::kProcessType);
  // Only append arguments for browser process.
  if (!process_type.empty())
    return;
}

content::ContentRendererClient* MainDelegate::CreateContentRendererClient() {
#if defined(CHROME_MULTIPLE_DLL_BROWSER)
  return NULL;
#else
  return new MesonRendererClient();
#endif
}
}
