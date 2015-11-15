#ifndef DEF_L_Array
#define DEF_L_Array

#include <cstdlib>
#include <cstring>
#include <functional>
#include "../objects.h"
#include "../streams/Stream.h"

namespace L {
  template <class T>
  class Array {
    private:
      void* _data;
      size_t _size, _capacity;

      inline void shift(size_t i, int offset) {
        memmove(&operator[](i+offset),&operator[](i),(_size-i)*sizeof(T));
      }

    public:
      Array() : _data(NULL), _size(0), _capacity(0) {}
      Array(const T* a, size_t size) : _data(NULL), _size(0), _capacity(0) {
        capacity(size);
        while(_size<size)
          push(*a++);
      }
      template <typename... Args>
      Array(size_t s, Args&&... args) : _data(NULL), _size(0), _capacity(0) {
        size(s,args...);
      }
      Array(const Array& other) : _size(other._size), _capacity(other._size) {
        _data = malloc(_size*sizeof(T));
        copy(&operator[](0),&other[0],_size);
      }
      ~Array() {
        destruct((T*)_data,_size);
        free(_data);
      }
      inline Array& operator=(const Array& other) {
        if(this!=&other) {
          reconstruct(*this,other);
        }
        return *this;
      }
      inline Array operator+(const Array& other) {Array wtr(*this); return wtr += other;}
      inline Array& operator+=(const Array& other) {insertArray(_size,other); return *this;}

      inline size_t size() const {return _size;}
      inline size_t capacity() const {return _capacity;}
      inline bool empty() const {return size()==0;}
      inline void clear() {size(0);}
      inline T& operator[](size_t i) {return *(((T*)_data)+i);}
      inline const T& operator[](size_t i) const {return *(((T*)_data)+i);}
      inline T& front() {return operator[](0);}
      inline const T& front() const {return operator[](0);}
      inline T& back() {return operator[](_size-1);}
      inline const T& back() const {return operator[](_size-1);}
      template <typename... Args> inline void push(Args&&... args) {insert(_size,args...);}
      template <typename... Args> inline void pushFront(Args&&... args) {insert(0,args...);}
      inline void pop() {erase(_size-1);}

      template <typename... Args>
      void size(size_t n,Args&&... args) {
        if(_capacity<n) growTo(n);
        if(_size<n) construct(&operator[](_size),n-_size,args...);
        else destruct(&operator[](n),_size-n);
        _size = n;
      }
      void capacity(size_t n) {
        if(n<_size) size(n); // Have to resize because capacity cannot be below size
        _data = realloc(_data,n*sizeof(T));
        _capacity = n;
      }
      void growTo(size_t size) {
        size_t tmp((_capacity)?_capacity:1);
        while(size>tmp) tmp *= 2;
        capacity(tmp);
      }
      inline void shrink() {capacity(size());}
      template <typename... Args>
      void insert(size_t i, Args&&... args) {
        growTo(_size+1); // Check capacity
        shift(i,1); // Move right part
        construct(operator[](i),args...); // Place new value
        _size++; // Increase size
      }
      inline void insertArray(size_t i, const Array& a, int alen=-1, size_t ai=0) {replaceArray(i,0,a,alen,ai);}
      template <typename... Args> inline void replace(size_t i, Args&&... args) {reconstruct(operator[](i),args...);}
      void replaceArray(size_t i, int len, const Array& a, int alen=-1, size_t ai=0) {
        if(alen==-1) alen = a.size();
        growTo(_size+(alen-len)); // Check capacity
        shift(i+len,alen-len);
        copy(&operator[](i),&a[ai],alen);
        _size += alen-len;
      }
      void erase(size_t i) {
        destruct(operator[](i)); // Destruct value
        shift(i+1,-1); // Move right part
        _size--; // Decrease size
      }
      void erase(size_t i, size_t count) {
        destruct(&operator[](i),count); // Destruct values
        shift(i+count,-count); // Move right part
        _size -= count; // Decrease size
      }

      void foreach(const std::function<void(const T&)>& f) const {
        for(size_t i(0); i<_size; i++)
          f(operator[](i));
      }
      void foreach(const std::function<void(T&)>& f) {
        for(size_t i(0); i<_size; i++)
          f(operator[](i));
      }

      template <typename... Args>
      inline static Array make(Args&&... args) {
        Array wtr;
        staticPush(wtr,args...);
        return wtr;
      }
      template <typename... Args>
      inline static void staticPush(Array& a, const T& e, Args&&... args) {
        a.push(e);
        staticPush(a,args...);
      }
      inline static void staticPush(Array& a) {}
  };

  template <class T>
  Stream& operator<<(Stream& s, const Array<T>& v) {
    s << '[';
    bool first(true);
    for(int i(0); i<v.size(); i++) {
      if(first) first = false;
      else s << ',';
      s << v[i];
    }
    s << ']';
    return s;
  }
}

#endif


