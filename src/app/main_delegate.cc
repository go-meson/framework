#include "main_delegate.h"

#include "base/logging.h"
#include "base/command_line.h"
#include "content/public/common/content_switches.h"

#include "common/options_switches.h"
#include "browser/browser_client.h"
#include "browser/relauncher.h"
#include "renderer/renderer_client.h"
#include "app/content_client.h"

#include <cstdio>

namespace {
const char* kRelauncherProcess = "relauncher";
bool IsBrowserProcess(base::CommandLine* cmd) {
  std::string process_type = cmd->GetSwitchValueASCII(::switches::kProcessType);
  return process_type.empty();
}
}

namespace meson {
MainDelegate::MainDelegate() {}
MainDelegate::~MainDelegate() {}

bool MainDelegate::BasicStartupComplete(int* exit_code) {
  auto command_line = base::CommandLine::ForCurrentProcess();

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);
  logging::SetLogItems(true, false, true, false);
  logging::SetMinLogLevel(logging::LOG_INFO);

  DLOG(INFO) << __PRETTY_FUNCTION__ << (IsBrowserProcess(command_line) ? "[Browser]" : "Renderer");
#if defined(OS_MACOSX)
  SetUpBundleOverrides();
#endif

  return brightray::MainDelegate::BasicStartupComplete(exit_code);
}

void MainDelegate::PreSandboxStartup(void) {
  brightray::MainDelegate::PreSandboxStartup();
  auto command_line = base::CommandLine::ForCurrentProcess();
  auto process_type = command_line->GetSwitchValueASCII(::switches::kProcessType);

  DLOG(INFO) << __PRETTY_FUNCTION__ << (IsBrowserProcess(command_line) ? "[Browser]" : "Renderer") << " / " << process_type;

  if (!IsBrowserProcess(command_line))
    return;

  // Only append arguments for browser process.
  if (!process_type.empty())
    return;

  if (command_line->HasSwitch(switches::kEnableSandbox)) {
    // Disable setuid sandbox since it is not longer required on linux(namespace
    // sandbox is available on most distros).
    command_line->AppendSwitch(::switches::kDisableSetuidSandbox);
  } else {
    // Disable renderer sandbox for most of node's functions.
    command_line->AppendSwitch(::switches::kNoSandbox);
  }

  // Allow file:// URIs to read other file:// URIs by default.
  command_line->AppendSwitch(::switches::kAllowFileAccessFromFiles);

  //command_line->AppendSwitch(::switches::kSingleProcess);//TODO

#if defined(OS_MACOSX)
  // Enable AVFoundation.
  command_line->AppendSwitch("enable-avfoundation");
#endif
}

content::ContentBrowserClient* MainDelegate::CreateContentBrowserClient() {
  browser_client_.reset(new meson::MesonBrowserClient);
  return browser_client_.get();
}

content::ContentRendererClient* MainDelegate::CreateContentRendererClient() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableSandbox)) {
    LOG(ERROR) << "Not Implement yet in kEnableSandbox";
    //renderer_client_.reset(new AtomSandboxedRendererClient);
  } else {
    renderer_client_.reset(new MesonRendererClient);
  }

  return renderer_client_.get();
}

#if 0
content::ContentUtilityClient* MainDelegate::CreateContentUtilityClient() {
  utility_client_.reset(new AtomContentUtilityClient);
  return utility_client_.get();
}
#endif

int MainDelegate::RunProcess(const std::string& process_type, const content::MainFunctionParams& main_function_params) {
  DLOG(INFO) << __PRETTY_FUNCTION__ << " : " << process_type;
  if (process_type == kRelauncherProcess)
    return relauncher::RelauncherMain(main_function_params);
  else
    return -1;
}

#if defined(OS_MACOSX)
bool MainDelegate::ShouldSendMachPort(const std::string& process_type) {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  return process_type != kRelauncherProcess;
}

bool MainDelegate::DelaySandboxInitialization(const std::string& process_type) {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  return process_type == kRelauncherProcess;
}
#endif

std::unique_ptr<brightray::ContentClient> MainDelegate::CreateContentClient() {
  DLOG(INFO) << __PRETTY_FUNCTION__;
  return std::unique_ptr<meson::ContentClient>(new meson::ContentClient());
}
}
