//-*-c++-*-
#pragma once
#include <memory>
#include "base/values.h"
#include "content/public/renderer/render_thread_observer.h"

namespace meson {
class PreferencesManager : public content::RenderThreadObserver {
 public:
  PreferencesManager();
  ~PreferencesManager() override;

 public:
  const base::ListValue* preferences() const { return preferences_.get(); }

 private:  // content::RenderThreadObserver:
  bool OnControlMessageReceived(const IPC::Message& message) override;
  void OnUpdatePreferences(const base::ListValue& preferences);

 private:
  std::unique_ptr<base::ListValue> preferences_;

  DISALLOW_COPY_AND_ASSIGN(PreferencesManager);
};
}
