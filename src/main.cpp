#include "L.h"
#include "engine/Resource.inl"

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
int main(int argc, const char* argv[]) {
  TypeInit();

#if L_DEBUG
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

  {
    L_SCOPE_MARKER("Initializing modules");
#include "module/init.gen" // Generated by premake5.lua
  }

  {
    ScriptContext ini_context;
    ini_context.execute(*Resource<ScriptFunction>("ini.ls"));
  }

  const char* window_name(Settings::get_string("window_name", "L Engine Sample"));
  int window_flags((Settings::get_int("no_cursor", 0) ? Window::nocursor : 0));

  if(Settings::get_int("fullscreen", 1))
    Window::instance()->open_fullscreen(window_name, window_flags);
  else
    Window::instance()->open(window_name, Settings::get_int("resolution_x", 1024), Settings::get_int("resolution_y", 768), window_flags);

  TaskSystem::push(mainjob, nullptr, uint32_t(-1), TaskSystem::MainTask);
  TaskSystem::init();
  return 0;
}

#if !defined(L_DEBUG) && defined(L_WINDOWS)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
  main(0, nullptr);
}
#endif
