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
class MesonMenuBinding : public APIBindingT<MesonMenuBinding>, public MesonMenuModel::Delegate {
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
  MesonMenuBinding(unsigned int id, const api::APICreateArg& args);
  virtual ~MesonMenuBinding(void);

#if defined(OS_MACOSX)
  static void SetApplicationMenu(MesonMenuBinding* menu);             // Set the global menubar.
  static void SendActionToFirstResponder(const std::string& action);  // Fake sending an action from the application menu.
#endif
  MesonMenuModel* model() const { return model_.get(); }

 public:  // Local Methods
  //TODO:
  MethodResult LoadTemplate(const api::APIArgs& args);
  MethodResult InsertItemAt(const api::APIArgs& args);
  MethodResult InsertSeparatorAt(const api::APIArgs& args);
  MethodResult InsertCheckItemAt(const api::APIArgs& args);
  MethodResult InsertRadioItemAt(const api::APIArgs& args);
  MethodResult InsertSubMenuAt(const api::APIArgs& args);
  //void SetIcon(int index, const gfx::Image& image);
  MethodResult SetSublabel(const api::APIArgs& args);
  MethodResult SetRole(const api::APIArgs& args);
  MethodResult Clear();
  MethodResult GetIndexOfCommandId(const api::APIArgs& args);
  MethodResult GetItemCount() const;
  MethodResult GetCommandIdAt(const api::APIArgs& args) const;
  MethodResult GetLabelAt(const api::APIArgs& args) const;
  MethodResult GetSublabelAt(const api::APIArgs& args) const;
  MethodResult IsItemCheckedAt(const api::APIArgs& args) const;
  MethodResult IsEnabledAt(const api::APIArgs& args) const;
  MethodResult IsVisibleAt(const api::APIArgs& args) const;

 public:  // MesonMenuModel::Delegate
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool IsCommandIdVisible(int command_id) const override;
  bool GetAcceleratorForCommandIdWithParams(int command_id, bool use_default_accelerator, ui::Accelerator* accelerator) const override;
  void ExecuteCommand(int command_id, int event_flags) override;
  void MenuWillShow(ui::SimpleMenuModel* source) override;
  virtual void PopupAt(MesonWindowBinding* window, int x = -1, int y = -1, int positioning_item = 0) = 0;

 protected:
  std::unique_ptr<MesonMenuModel> model_;
  base::WeakPtr<MesonMenuBinding> parent_;
  std::map<int,ui::Accelerator> accelerators_;
  std::map<int,std::string> clickEvents_;
 private:
};

class MesonMenuFactory : public APIBindingFactory {
public:
  MesonMenuFactory(void);
  virtual ~MesonMenuFactory(void);
public:
  APIBinding* Create(unsigned int id, const api::APICreateArg& args) override;
private:
  DISALLOW_COPY_AND_ASSIGN(MesonMenuFactory);
};
}
