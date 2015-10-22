#ifndef DEF_L_Image_Color
#define DEF_L_Image_Color

#include <cstdlib>
#include "../macros.h"
#include "../Number.h"
#include "../String.h"
#include "../types.h"

namespace L {
  class Color {
    protected:
      byte _data[4];
    public:
      inline Color() {
        _data[0] =
          _data[1] =
            _data[2] =
              _data[3] = 0;
      }
      inline Color(byte gs) {
        _data[0] =
          _data[1] =
            _data[2] = gs;
        _data[3] = 0xFF;
      }
      inline Color(byte r, byte g, byte b, byte a = 0xFF) {
        _data[0] = r;
        _data[1] = g;
        _data[2] = b;
        _data[3] = a;
      }
      Color(String);

      bool operator==(const Color&) const;
      bool operator!=(const Color&) const;
      bool operator<(const Color&) const;
      Color& operator+=(const Color&);

      inline byte r() const {return _data[0];}
      inline byte g() const {return _data[1];}
      inline byte b() const {return _data[2];}
      inline byte a() const {return _data[3];}
      inline uint rgb() const {return r()<<16|g()<<8|b();}
      inline uint rgba() const {return r()<<24|g()<<16|b()<<8|a();}
      inline byte& r() {return _data[0];}
      inline byte& g() {return _data[1];}
      inline byte& b() {return _data[2];}
      inline byte& a() {return _data[3];}
      inline const byte* bytes() const {return _data;}

      static Color from(float r, float g, float b, float a = 1);
      static Color lerp(Color, Color, float w);
      static const Color black, blue, cyan, green, grey, lightgrey, magenta, red, white, yellow;
  };
}

#endif





