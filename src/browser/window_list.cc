#include "browser/window_list.h"

#include <algorithm>

#include "browser/native_window.h"
#include "browser/window_list_observer.h"
#include "base/logging.h"

namespace meson {

base::LazyInstance<base::ObserverList<WindowListObserver>>::Leaky WindowList::observers_ = LAZY_INSTANCE_INITIALIZER;
WindowList* WindowList::instance_ = nullptr;

WindowList* WindowList::GetInstance() {
  if (!instance_)
    instance_ = new WindowList;
  return instance_;
}

void WindowList::AddWindow(NativeWindow* window) {
  DCHECK(window);
  // Push |window| on the appropriate list instance.
  WindowVector& windows = GetInstance()->windows_;
  windows.push_back(window);

  FOR_EACH_OBSERVER(WindowListObserver, observers_.Get(), OnWindowAdded(window));
}

void WindowList::RemoveWindow(NativeWindow* window) {
  WindowVector& windows = GetInstance()->windows_;
  windows.erase(std::remove(windows.begin(), windows.end(), window), windows.end());

  FOR_EACH_OBSERVER(WindowListObserver, observers_.Get(), OnWindowRemoved(window));

  if (windows.size() == 0)
    FOR_EACH_OBSERVER(WindowListObserver, observers_.Get(), OnWindowAllClosed());
}

void WindowList::WindowCloseCancelled(NativeWindow* window) {
  FOR_EACH_OBSERVER(WindowListObserver, observers_.Get(), OnWindowCloseCancelled(window));
}

void WindowList::AddObserver(WindowListObserver* observer) {
  observers_.Get().AddObserver(observer);
}

void WindowList::RemoveObserver(WindowListObserver* observer) {
  observers_.Get().RemoveObserver(observer);
}

void WindowList::CloseAllWindows() {
  WindowVector windows = GetInstance()->windows_;
  for (const auto& window : windows)
    if (!window->IsClosed())
      window->Close();
}

WindowList::WindowList() {}

WindowList::~WindowList() {}

}
