//-*-objc-*-
#include "app/main_delegate.h"
#include "base/mac/bundle_locations.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/logging.h"
#include "content/public/common/content_paths.h"

#include "brightray/common/mac/main_application_bundle.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/strings/sys_string_conversions.h"


#include <cstdio>

namespace {
base::FilePath GetFrameworksPath() {
  // TODO: generate identifier in meson.gyp?
  NSBundle* bundle =
      [NSBundle bundleWithIdentifier:@"com.github.meson.framework"];
  return base::mac::NSStringToFilePath([bundle bundlePath]);
}
}

namespace meson {
void MainDelegate::OverrideFrameworkBundlePath() {
  auto path = GetFrameworksPath();
  base::mac::SetOverrideFrameworkBundlePath(path);
}

void MainDelegate::OverrideChildProcessPath() {
  auto framework = GetFrameworksPath();
  auto helper_path = framework.Append("..")
                         .Append("Meson Helper.app")
                         .Append("Contents")
                         .Append("MacOS")
                         .Append("Meson Helper");
  PathService::Override(content::CHILD_PROCESS_EXE, helper_path);
}

void MainDelegate::SetUpBundleOverrides() {
  base::mac::ScopedNSAutoreleasePool pool;
  NSBundle* bundle = brightray::MainApplicationBundle();
  std::string base_bundle_id = base::SysNSStringToUTF8([bundle bundleIdentifier]);
  NSString* team_id = [bundle objectForInfoDictionaryKey:@"MesonTeamID"];
  if (team_id)
    base_bundle_id = base::SysNSStringToUTF8(team_id) + "." + base_bundle_id;
  base::mac::SetBaseBundleID(base_bundle_id.c_str());
}

}
