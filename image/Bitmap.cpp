#include "Bitmap.h"

#include "../Interface.h"
#include "Vector.h"

using namespace L;
using namespace Image;

Bitmap::Bitmap(int width, int height) : Array<2,Color>(width,height) {}
Bitmap::Bitmap(int width, int height, const Color& c) : Array<2,Color>(width,height) {
  for(int x(0); x<width; x++)
    for(int y(0); y<height; y++)
      (*this)(x,y) = c;
}
Bitmap::Bitmap(int width, int height, const Image::Vector& v) : Array<2,Color>(width,height) {
  v.drawOn(*this);
}
Bitmap::Bitmap(const String& filePath) : Array<2,Color>() {
  Interface<Bitmap>::fromFile(*this,filePath);
}
Bitmap& Bitmap::load(const String& filePath) {
  Interface<Bitmap>::fromFile(*this,filePath);
  return *this;
}
Bitmap& Bitmap::save(const String& filePath) {
  Interface<Bitmap>::toFile(*this,filePath);
  return *this;
}

Bitmap Bitmap::sub(int x, int y, int width, int height) const {
  Bitmap wtr;
  wtr.resize(width,height);
  for(int i=0; i<width; i++)
    for(int j=0; j<height; j++)
      wtr(i,j) = (*this)(x+i,y+j);
  return wtr;
}
Bitmap Bitmap::filter(Color c) const {
  Bitmap wtr;
  wtr.resize(width(),height());
  float r = (float)c.r()/255.0,
        g = (float)c.g()/255.0,
        b = (float)c.b()/255.0,
        a = (float)c.a()/255.0;
  for(int x=0; x<width(); x++)
    for(int y=0; y<height(); y++) {
      Color tmp((*this)(x,y));
      wtr(x,y) = Color((float)tmp.r()*r,
                       (float)tmp.g()*g,
                       (float)tmp.b()*b,
                       (float)tmp.a()*a);
    }
  return wtr;
}
Bitmap Bitmap::trim(Color c) const {
  int left = 0, right = width()-1,
      top = 0, bottom = height()-1,
      x,y;
  // Left
  y = top;
  while(c == (*this)(left,y)) {
    if(y<bottom) y++;
    else {y = top; left++;}
  }
  // Top
  x = left;
  while(c == (*this)(x,top)) {
    if(x<right) x++;
    else {x = left; top++;}
  }
  // Right
  y = top;
  while(c == (*this)(right,y)) {
    if(y<bottom) y++;
    else {y = top; right--;}
  }
  // Bottom
  x = left;
  while(c == (*this)(x,bottom)) {
    if(x<right) x++;
    else {x = left; bottom--;}
  }
  return trim(left,right+1,top,bottom+1);
}
Bitmap Bitmap::trim(int left, int right, int top, int bottom) const {
  Bitmap wtr;
  wtr.resize(right-left,bottom-top);
  for(int x=left; x<right; x++)
    for(int y=top; y<bottom; y++)
      wtr(x-left,y-top) = (*this)(x,y);
  return wtr;
}
void Bitmap::scale(int newWidth, int newHeight) {
  float hf = (float)height()/(float)newHeight,
        wf = (float)width()/(float)newWidth;
  Bitmap copy(*this);
  resize(newWidth,newHeight);
  for(int x=0; x<newWidth; x++)
    for(int y=0; y<newHeight; y++)
      (*this)(x,y) = copy((float)x*wf,(float)y*hf);
}
void Bitmap::blur(int factor) {
  uint rt, gt, bt;
  float pixelCount, m;
  Color c;
  Bitmap copy(*this);
  for(int x = 0; x<(int)width(); x++)
    for(int y = 0; y<(int)height(); y++) {
      pixelCount = 0.0;
      rt = 0;
      gt = 0;
      bt = 0;
      for(int i=x-factor; i<=x+factor; i++)
        for(int j=y-factor; j<=y+factor; j++)
          if(i>=0 && i<(int)width()
              && j>=0 && j<(int)height()) {
            m = (float)factor-sqrt(pow((float)(x-i),2)+pow((float)(y-j),2));
            if(m>=1) {
              pixelCount+=m;
              c = copy(i,j);
              rt += c.r()*m;
              gt += c.g()*m;
              bt += c.b()*m;
            }
          }
      c.r() = rt / pixelCount;
      c.g() = gt / pixelCount;
      c.b() = bt / pixelCount;
      (*this)(x,y) = c;
    }
}
void Bitmap::drawTriangle(Surface<2,float> s,Color c) {
  Interval<2,float> interval(s.gA(),s.gB());
  interval.add(s.gC());
  Point<2,int> p;
  // float loop testing all pixels inside the interval
  for(p.x() = interval.min().x();
      p.x() < interval.max().x();
      p.x()++)
    for(p.y() = interval.min().y();
        p.y() < interval.max().y();
        p.y()++)
      if(s.contains(p))
        (*this)((int)p.x(),(int)p.y()) = c;
}
