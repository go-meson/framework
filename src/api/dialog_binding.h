#pragma once
#include "api/api_binding.h"

namespace meson {
class DialogClassBinding : public APIClassBindingT<DialogClassBinding, DialogClassBinding> {
public:
  DialogClassBinding(void);
  ~DialogClassBinding(void) override;

public:                         // Class Methods
  api::MethodResult ShowMessageBox(const api::APIArgs& args);
private:
  void ShowMessageBoxCallback(const std::string& eventName, int buttonId);

  DISALLOW_COPY_AND_ASSIGN(DialogClassBinding);
};
}
