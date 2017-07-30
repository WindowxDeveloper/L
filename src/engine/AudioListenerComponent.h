#pragma once

#include "../audio/Audio.h"
#include "Component.h"
#include "Transform.h"

namespace L {
  class AudioListenerComponent : public Component {
    L_COMPONENT(AudioListenerComponent)
  protected:
    Transform* _transform;
    Vector3f _last_position;
  public:
    void update();
    void updateComponents();
  };
}