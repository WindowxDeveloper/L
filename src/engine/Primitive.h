#pragma once

#include "Transform.h"

namespace L {
  class Primitive : public Component {
    L_COMPONENT(Primitive)
  protected:
    Transform* _transform;
    Vector3f _center,_radius;
    enum {
      Box,Sphere
    } _type;
  public:
    inline Primitive() : _center(0.f),_radius(1.f),_type(Box){}
    void updateComponents();
    inline void center(const Vector3f& c){ _center = c; }
    inline void box(const Vector3f& radius){ _type = Box; _radius = radius; }
    inline void sphere(float radius){ _type = Sphere; _radius = radius; }
    void render(const Camera& camera);
  };
}
