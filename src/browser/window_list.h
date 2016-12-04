//-*-c++-*-
#pragma once

#include <vector>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/observer_list.h"

namespace meson {
class NativeWindow;
class WindowListObserver;

class WindowList {
 public:
  typedef std::vector<NativeWindow*> WindowVector;
  typedef WindowVector::iterator iterator;
  typedef WindowVector::const_iterator const_iterator;

  const_iterator begin() const { return windows_.begin(); }
  const_iterator end() const { return windows_.end(); }

  iterator begin() { return windows_.begin(); }
  iterator end() { return windows_.end(); }

  bool empty() const { return windows_.empty(); }
  size_t size() const { return windows_.size(); }

  NativeWindow* get(size_t index) const { return windows_[index]; }

  static WindowList* GetInstance();

  static void AddWindow(NativeWindow* window);
  static void RemoveWindow(NativeWindow* window);

  static void WindowCloseCancelled(NativeWindow* window);

  static void AddObserver(WindowListObserver* observer);
  static void RemoveObserver(WindowListObserver* observer);

  static void CloseAllWindows();

 private:
  WindowList();
  ~WindowList();

  WindowVector windows_;

  static base::LazyInstance<base::ObserverList<WindowListObserver>>::Leaky observers_;

  static WindowList* instance_;

  DISALLOW_COPY_AND_ASSIGN(WindowList);
};

}  // namespace atom
