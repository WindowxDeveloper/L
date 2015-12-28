#ifndef DEF_L_Component
#define DEF_L_Component

namespace L {
  class Entity;
  class Component {
    private:
      Entity* _entity;
      inline void entity(Entity* e) {_entity = e;}
    public:
      inline Entity& entity() const {return *_entity;}
      virtual void start() {}
      static inline void preupdates() {}

      virtual Component* clone() = 0;

      friend class Entity;
  };
}

#define L_COMPONENT(name)\
  Component* clone() {return Pool<name>::global.construct(*this);}

#include "Entity.h"

#endif

