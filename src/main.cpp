#include "engine/Engine.h"
#include "engine/debug_draw.h"
#include "engine/Resource.inl"
#include "engine/Settings.h"
#include "network/Network.h"

#include "component/AudioListenerComponent.h"
#include "component/AudioSourceComponent.h"
#include "component/Camera.h"
#include "component/Collider.h"
#include "component/GUIComponent.h"
#include "component/HierarchyComponent.h"
#include "component/MidiSourceComponent.h"
#include "component/NameComponent.h"
#include "component/PostProcessComponent.h"
#include "component/Primitive.h"
#include "component/RigidBody.h"
#include "component/ScriptComponent.h"
#include "component/SkeletalAnimatorComponent.h"
#include "component/Transform.h"

using namespace L;

void mainjob(void*) {
  Network::init();

  while(Window::opened()) {
    Engine::update();
  }
  Engine::clear();
  TaskSystem::join_all();
#ifdef L_DEBUG
  flush_profiling();
#endif
}
int main(int, const char*[]) {
  TypeInit();

#if L_DEBUG
  init_debug_draw();
  Engine::add_parallel_update(ResourceSlot::update);
#endif

  Engine::register_component<Transform>();
  Engine::register_component<Camera>();
  Engine::register_component<RigidBody>();
  Engine::register_component<Collider>();
  Engine::register_component<ScriptComponent>();
  Engine::register_component<AudioSourceComponent>();
  Engine::register_component<AudioListenerComponent>();
  Engine::register_component<MidiSourceComponent>();
  Engine::register_component<NameComponent>();
  Engine::register_component<HierarchyComponent>();
  Engine::register_component<PostProcessComponent>();
  Engine::register_component<Primitive>();
  Engine::register_component<GUIComponent>();
  Engine::register_component<SkeletalAnimatorComponent>();

  {
    L_SCOPE_MARKER("Initializing modules");
#include "module/init.gen" // Generated by premake5.lua
  }

  {
    Resource<ScriptFunction> ini_script = "ini.ls";
    ini_script.flush();
    if(ini_script.is_loaded()) {
      ScriptContext().execute(ref<ScriptFunction>(*ini_script));
    } else {
      error("Could not load init script");
    }
  }

  const char* window_name(Settings::get_string("window_name", "L Engine Sample"));

  uint32_t window_flags = 0;
  window_flags |= Settings::get_int("no_cursor", 0) ? Window::nocursor : 0;
  window_flags |= Settings::get_int("resizable_window", 0) ? Window::resizable : 0;

  if(Settings::get_int("fullscreen", 1))
    Window::instance()->open_fullscreen(window_name, window_flags);
  else
    Window::instance()->open(window_name, Settings::get_int("resolution_x", 1024), Settings::get_int("resolution_y", 768), window_flags);

  TaskSystem::push(mainjob, nullptr, uint32_t(-1), TaskSystem::MainTask);
  TaskSystem::init();
  return 0;
}

#if !defined(L_DEBUG) && defined(L_WINDOWS)
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  main(0, nullptr);
}
#endif
