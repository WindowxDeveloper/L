#ifndef DEF_L_StaticStack
#define DEF_L_StaticStack

#include "../Object.h"
#include "../types.h"

namespace L {
  template <int n, class T>
  class StaticStack {
    private:
      byte _array[n*sizeof(T)];
      T* _current;

    public:
      StaticStack() : _current(((T*)_array)-1) {}
      template <class... Args>
      inline void push(Args&&... args) {
        _current++;
        Object::construct(*_current,args...);
      }
      inline void pop() {
        Object::destruct(*_current);
        _current--;
      }
      inline T& top() {
        return *_current;
      }
  };
}

#endif


