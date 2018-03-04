#pragma once

#include <initializer_list>
#include "GL.h"
#include "../macros.h"

namespace L {
  class MeshBuilder;
  class Mesh {
    L_NOCOPY(Mesh)
  private:
    GLuint _vao, _vbo, _eab;
    GLenum _mode;
    GLsizei _count;
  public:
    struct Attribute {
      GLint size;
      GLenum type;
      GLboolean normalized;
    };
    inline Mesh() : _vao(0), _vbo(0), _eab(0) {}
    Mesh(GLenum mode, GLsizei count, const void* data, GLsizeiptr size, const std::initializer_list<Attribute>&, const GLushort* indices = nullptr, GLsizei icount = 0);
    Mesh(const MeshBuilder&, GLenum mode, const std::initializer_list<Attribute>&);
    ~Mesh();
    void indices(const GLushort* data, GLsizei count);
    void load(GLenum mode, GLsizei count, const void* data, GLsizeiptr size, const std::initializer_list<Attribute>& attributes, const GLushort* indices = nullptr, GLsizei icount = 0);
    void load(const MeshBuilder&, GLenum mode, const std::initializer_list<Attribute>&);
    void draw() const;

    static const Mesh& quad();
    static const Mesh& wire_cube();
    static const Mesh& wire_sphere();
  };
}