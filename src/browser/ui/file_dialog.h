//-*-c++-*-
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"

namespace meson {
class NativeWindow;
}

namespace file_dialog {

// <description, extensions>
typedef std::pair<std::string, std::vector<std::string>> Filter;
typedef std::vector<Filter> Filters;

enum FileDialogProperty {
  FILE_DIALOG_OPEN_FILE = 1 << 0,
  FILE_DIALOG_OPEN_DIRECTORY = 1 << 1,
  FILE_DIALOG_MULTI_SELECTIONS = 1 << 2,
  FILE_DIALOG_CREATE_DIRECTORY = 1 << 3,
  FILE_DIALOG_SHOW_HIDDEN_FILES = 1 << 4,
};

typedef base::Callback<void(
    bool result,
    const std::vector<base::FilePath>& paths)>
    OpenDialogCallback;

typedef base::Callback<void(
    bool result,
    const base::FilePath& path)>
    SaveDialogCallback;

bool ShowOpenDialog(meson::NativeWindow* parent_window,
                    const std::string& title,
                    const std::string& button_label,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    int properties,
                    std::vector<base::FilePath>* paths);

void ShowOpenDialog(meson::NativeWindow* parent_window,
                    const std::string& title,
                    const std::string& button_label,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    int properties,
                    const OpenDialogCallback& callback);

bool ShowSaveDialog(meson::NativeWindow* parent_window,
                    const std::string& title,
                    const std::string& button_label,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    base::FilePath* path);

void ShowSaveDialog(meson::NativeWindow* parent_window,
                    const std::string& title,
                    const std::string& button_label,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    const SaveDialogCallback& callback);
}
