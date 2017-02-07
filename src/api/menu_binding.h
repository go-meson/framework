//-*-c++-*-
#pragma once

#include <memory>
#include <string>
#include <map>

#include "api/api_binding.h"
#include "api/window_binding.h"
#include "browser/ui/meson_menu_model.h"
#include "base/memory/ref_counted.h"
#include "base/callback.h"

namespace meson {
class MenuClassBinding;
class MenuBinding : public APIBindingT<MenuBinding, MenuClassBinding>, public MesonMenuModel::Delegate {
  friend class MenuClassBinding;

 public:
  enum { ObjType = MESON_OBJECT_TYPE_MENU };

 public:
  class MenuItem : base::RefCountedThreadSafe<MenuItem> {
   public:
    MenuItem(void);
    virtual ~MenuItem(void);

   public:
    bool isChecked;
    bool isEnable;
    bool isVisible;
  };

 public:
  MenuBinding(api::ObjID id);
  virtual ~MenuBinding(void);

#if defined(OS_MACOSX)
  static void SendActionToFirstResponder(const std::string& action);  // Fake sending an action from the application menu.
#endif
  MesonMenuModel* model() const { return model_.get(); }

 public:  // Local Methods
  //TODO:
  api::MethodResult LoadTemplate(const api::APIArgs& args);
  api::MethodResult InsertItemAt(const api::APIArgs& args);
  api::MethodResult InsertSeparatorAt(const api::APIArgs& args);
  api::MethodResult InsertCheckItemAt(const api::APIArgs& args);
  api::MethodResult InsertRadioItemAt(const api::APIArgs& args);
  api::MethodResult InsertSubMenuAt(const api::APIArgs& args);
  //void SetIcon(int index, const gfx::Image& image);
  api::MethodResult SetSublabel(const api::APIArgs& args);
  api::MethodResult SetRole(const api::APIArgs& args);
  api::MethodResult Clear();
  api::MethodResult GetIndexOfCommandId(const api::APIArgs& args);
  api::MethodResult GetItemCount() const;
  api::MethodResult GetCommandIdAt(const api::APIArgs& args) const;
  api::MethodResult GetLabelAt(const api::APIArgs& args) const;
  api::MethodResult GetSublabelAt(const api::APIArgs& args) const;
  api::MethodResult IsItemCheckedAt(const api::APIArgs& args) const;
  api::MethodResult IsEnabledAt(const api::APIArgs& args) const;
  api::MethodResult IsVisibleAt(const api::APIArgs& args) const;

 public:  // MesonMenuModel::Delegate
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool IsCommandIdVisible(int command_id) const override;
  bool GetAcceleratorForCommandIdWithParams(int command_id, bool use_default_accelerator, ui::Accelerator* accelerator) const override;
  void ExecuteCommand(int command_id, int event_flags) override;
  void MenuWillShow(ui::SimpleMenuModel* source) override;
  virtual void PopupAt(WindowBinding* window, int x = -1, int y = -1, int positioning_item = 0) = 0;

 protected:
  std::unique_ptr<MesonMenuModel> model_;
  base::WeakPtr<MenuBinding> parent_;
  std::map<int, ui::Accelerator> accelerators_;
  std::map<int, std::string> clickEvents_;

 private:
};

class MenuClassBinding : public APIClassBindingT<MenuBinding, MenuClassBinding> {
 public:
  MenuClassBinding();
  ~MenuClassBinding() override;

 public:  // static methods
  //TODO:
  api::MethodResult CreateInstance(const api::APIArgs& args);
#if defined(OS_MACOSX)
  api::MethodResult SetApplicationMenu(const api::APIArgs& args);  // Set the global menubar.
#endif
  DISALLOW_COPY_AND_ASSIGN(MenuClassBinding);
};
}
