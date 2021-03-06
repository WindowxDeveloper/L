#pragma once

#include "Component.h"
#include "../script/ScriptContext.h"
#include "../system/Device.h"
#include "../engine/Resource.h"

namespace L {
  class ScriptComponent : public TComponent<ScriptComponent,
    ComponentFlag::Update | ComponentFlag::LateUpdate | ComponentFlag::DeviceEvent> {
  protected:
    ScriptContext _context;
    Resource<ScriptFunction> _script;
    bool _started : 1;
  public:
    ScriptComponent() : _started(false) {}
    virtual void update_components() override;
    virtual Map<Symbol, Var> pack() const override;
    virtual void unpack(const Map<Symbol, Var>&) override;
    static void script_registration();

    void load(const char* filename);
    void start();
    void update();
    void late_update();
    void event(const Device::Event&);
    void event(const Ref<Table<Var, Var>>&);
  };
}
