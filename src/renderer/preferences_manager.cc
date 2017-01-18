#include "renderer/preferences_manager.h"

#include "content/public/renderer/render_thread.h"
#include "api/api_messages.h"

namespace meson {
PreferencesManager::PreferencesManager() {
  content::RenderThread::Get()->AddObserver(this);
}

PreferencesManager::~PreferencesManager() {
}

bool PreferencesManager::OnControlMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PreferencesManager, message)
    IPC_MESSAGE_HANDLER(MesonMsg_UpdatePreferences, OnUpdatePreferences)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PreferencesManager::OnUpdatePreferences(const base::ListValue& preferences) {
  preferences_ = preferences.CreateDeepCopy();
}
}
