// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "common/meson_command_line.h"

#include "base/command_line.h"

namespace meson {

// static
std::vector<std::string> MesonCommandLine::argv_;

#if defined(OS_WIN)
// static
std::vector<std::wstring> MesonCommandLine::wargv_;
#endif

// static
void MesonCommandLine::Init(int argc, const char* const* argv) {
  // Hack around with the argv pointer. Used for process.title = "blah"
  //char** new_argv = uv_setup_args(argc, const_cast<char**>(argv));
  for (int i = 0; i < argc; ++i) {
    argv_.push_back(argv[i]);
  }
}

#if defined(OS_WIN)
// static
void MesonCommandLine::InitW(int argc, const wchar_t* const* argv) {
  for (int i = 0; i < argc; ++i) {
    wargv_.push_back(argv[i]);
  }
}
#endif

#if defined(OS_LINUX)
// static
void MesonCommandLine::InitializeFromCommandLine() {
  argv_ = base::CommandLine::ForCurrentProcess()->argv();
}
#endif
}
