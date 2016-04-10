#include "Engine.h"

#include "Camera.h"

using namespace L;

Set<void (*)()> Engine::_updates;
Set<void (*)(const Camera&)> Engine::_renders;
Map<String,Ref<GL::Texture> > Engine::_textures;
Map<String,Ref<GL::Mesh> > Engine::_meshes;
Timer Engine::_timer;
Time Engine::_deltaTime;
float Engine::_deltaSeconds, Engine::_fps;
uint Engine::_frame(0);

void Engine::update() {
  _deltaTime = _timer.frame();
  _deltaSeconds = _deltaTime.fSeconds();
  _fps = 1.f/_deltaSeconds;
  for(int i(0); i<_updates.size(); i++)
    _updates[i]();
  for(auto&& camera : Pool<Camera>::global){
    camera.prerender();
    for(int i(0); i<_renders.size(); i++)
      _renders[i](camera);
  }
  _frame++;
}
const Ref<GL::Texture>& Engine::texture(const char* fp) {
  String filepath(fp);
  if(_textures.has(filepath))
    return _textures[filepath];
  else return _textures[filepath] = new GL::Texture(Bitmap(filepath));
}
const Ref<GL::Mesh>& Engine::mesh(const char* fp) {
  String filepath(fp);
  if(_meshes.has(filepath))
    return _meshes[filepath];
  else return _meshes[filepath] = new GL::Mesh(filepath);
}
