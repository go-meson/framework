#pragma once

namespace meson {
    class NativeWindow;
    class WindowListObserver {
    public:
    // Called immediately after a window is added to the list.
    virtual void OnWindowAdded(NativeWindow* window) {}

    // Called immediately after a window is removed from the list.
    virtual void OnWindowRemoved(NativeWindow* window) {}

    // Called when a window close is cancelled by beforeunload handler.
    virtual void OnWindowCloseCancelled(NativeWindow* window) {}

    // Called immediately after all windows are closed.
    virtual void OnWindowAllClosed() {}

    protected:
    virtual ~WindowListObserver() {}
    };
}