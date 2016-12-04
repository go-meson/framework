//-*-c++-*-
#pragma once

#include <string>
#include <vector>

#include "browser/browser_observer.h"
#include "browser/window_list_observer.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "base/values.h"

#if defined(OS_WIN)
#include "base/files/file_path.h"
#endif

namespace base {
class FilePath;
}

namespace gfx {
class Image;
}

namespace meson {

class MesonMenuModel;
class LoginHandler;

// This class is used for control application-wide operations.
class Browser : public WindowListObserver {
 public:
  Browser();
  ~Browser();

  static Browser* Get();

  void Quit();                                         // Try to close all windows and quit the application.
  void Exit(int code);                                 // Exit the application immediately and set exit code.
  void Shutdown();                                     // Cleanup everything and shutdown the application gracefully.
  void Focus();                                        // Focus the application.
  std::string GetVersion() const;                      // Returns the version of the executable (or bundle).
  void SetVersion(const std::string& version);         // Overrides the application version.
  std::string GetName() const;                         // Returns the application's name, default is just Electron.
  void SetName(const std::string& name);               // Overrides the application name.
  void AddRecentDocument(const base::FilePath& path);  // Add the |path| to recent documents list.
  void ClearRecentDocuments();                         // Clear the recent documents list.
  void SetAppUserModelID(const base::string16& name);  // Set the application user model ID.

#if 0
  // Remove the default protocol handler registry key
  bool RemoveAsDefaultProtocolClient(const std::string& protocol,
                                     mate::Arguments* args);

  // Set as default handler for a protocol.
  bool SetAsDefaultProtocolClient(const std::string& protocol,
                                  mate::Arguments* args);

  // Query the current state of default handler for a protocol.
  bool IsDefaultProtocolClient(const std::string& protocol,
                               mate::Arguments* args);
#endif

  // Set/Get the badge count.
  bool SetBadgeCount(int count);
  int GetBadgeCount();

  // Set/Get the login item settings of the app
  struct LoginItemSettings {
    bool open_at_login = false;
    bool open_as_hidden = false;
    bool restore_state = false;
    bool opened_at_login = false;
    bool opened_as_hidden = false;
  };
  void SetLoginItemSettings(LoginItemSettings settings);
  LoginItemSettings GetLoginItemSettings();

#if defined(OS_MACOSX)
  // Hide the application.
  void Hide();

  // Show the application.
  void Show();

#if 0
  // Creates an activity and sets it as the one currently in use.
  void SetUserActivity(const std::string& type,
                       const base::DictionaryValue& user_info,
                       mate::Arguments* args);
#endif

  // Returns the type name of the current user activity.
  std::string GetCurrentActivityType();

  // Resumes an activity via hand-off.
  bool ContinueUserActivity(const std::string& type,
                            const base::DictionaryValue& user_info);

  // Bounce the dock icon.
  enum BounceType {
    BOUNCE_CRITICAL = 0,
    BOUNCE_INFORMATIONAL = 10,
  };
  int DockBounce(BounceType type);
  void DockCancelBounce(int request_id);

  // Bounce the Downloads stack.
  void DockDownloadFinished(const std::string& filePath);

  // Set/Get dock's badge text.
  void DockSetBadgeText(const std::string& label);
  std::string DockGetBadgeText();

  // Hide/Show dock.
  void DockHide();
  void DockShow();
  bool DockIsVisible();

  // Set docks' menu.
  void DockSetMenu(MesonMenuModel* model);

  // Set docks' icon.
  void DockSetIcon(const gfx::Image& image);

  void ShowAboutPanel();
  void SetAboutPanelOptions(const base::DictionaryValue& options);
#endif  // defined(OS_MACOSX)

#if defined(OS_WIN)
  struct UserTask {
    base::FilePath program;
    base::string16 arguments;
    base::string16 title;
    base::string16 description;
    base::FilePath icon_path;
    int icon_index;
  };

  // Add a custom task to jump list.
  bool SetUserTasks(const std::vector<UserTask>& tasks);

  // Returns the application user model ID, if there isn't one, then create
  // one from app's name.
  // The returned string managed by Browser, and should not be modified.
  PCWSTR GetAppUserModelID();
#endif  // defined(OS_WIN)

#if defined(OS_LINUX)
  // Whether Unity launcher is running.
  bool IsUnityRunning();
#endif  // defined(OS_LINUX)

  // Tell the application to open a file.
  bool OpenFile(const std::string& file_path);

  // Tell the application to open a url.
  void OpenURL(const std::string& url);

  // Tell the application that application is activated with visible/invisible
  // windows.
  void Activate(bool has_visible_windows);

  // Tell the application the loading has been done.
  void WillFinishLaunching();
  void DidFinishLaunching(const base::DictionaryValue& launch_info);

  void OnAccessibilitySupportChanged();

  // Request basic auth login.
  void RequestLogin(LoginHandler* login_handler,
                    std::unique_ptr<base::DictionaryValue> request_details);

  void AddObserver(BrowserObserver* obs) {
    observers_.AddObserver(obs);
  }

  void RemoveObserver(BrowserObserver* obs) {
    observers_.RemoveObserver(obs);
  }

  bool is_shutting_down() const { return is_shutdown_; }
  bool is_quiting() const { return is_quiting_; }
  bool is_ready() const { return is_ready_; }

 protected:
  // Returns the version of application bundle or executable file.
  std::string GetExecutableFileVersion() const;

  // Returns the name of application bundle or executable file.
  std::string GetExecutableFileProductName() const;

  // Send the will-quit message and then shutdown the application.
  void NotifyAndShutdown();

  // Send the before-quit message and start closing windows.
  bool HandleBeforeQuit();

  bool is_quiting_;

 private:
  // WindowListObserver implementations:
  void OnWindowCloseCancelled(NativeWindow* window) override;
  void OnWindowAllClosed() override;

  // Observers of the browser.
  base::ObserverList<BrowserObserver> observers_;

  // Whether `app.exit()` has been called
  bool is_exiting_;

  // Whether "ready" event has been emitted.
  bool is_ready_;

  // The browser is being shutdown.
  bool is_shutdown_;

  std::string version_override_;
  std::string name_override_;

  int badge_count_ = 0;

#if defined(OS_WIN)
  base::string16 app_user_model_id_;
#endif

#if defined(OS_MACOSX)
  base::DictionaryValue about_panel_options_;
#endif

  DISALLOW_COPY_AND_ASSIGN(Browser);
};
}
