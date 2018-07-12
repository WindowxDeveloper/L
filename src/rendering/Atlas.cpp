#include "Atlas.h"

using namespace L;

Atlas::Atlas(uint32_t width, uint32_t height)
  : Texture(width, height), _xs {0}, _ys {0} {
}
Interval2f Atlas::add(uint32_t part_width, uint32_t part_height, const void* data) {
  Vector2i i(0, 0);
  while(true) {
    const Vector2i pos(_xs[i.x()], _ys[i.y()]);
    const Interval2i candidate(pos, pos+Vector2i(part_width, part_height));
    const bool valid(candidate.max().x()<width() && candidate.max().y()<height() && !_parts.overlaps(candidate));

    if(valid) {
      load(data, VkOffset3D {candidate.min().x(), candidate.min().y()}, VkExtent3D {part_width, part_height, 1});
      _parts.insert(candidate, true);
      _xs.insert(candidate.max().x());
      _ys.insert(candidate.max().y());
      return pixelToCoords(candidate);
    } else if(candidate.max().y()>=height())
      break;
    else {
      if(++i.x()==_xs.size()) {
        i.x() = 0;
        if(++i.y()==_ys.size())
          break; // No more y steps
      }
    }
  }
  return Interval2f();
}
Interval2f Atlas::pixelToCoords(const Interval2i& i) const {
  const Vector2f ratio(1.f/width(), 1.f/height());
  return Interval2f(ratio*i.min(), ratio*i.max());
}
