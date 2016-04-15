#include "MeshBuilder.h"

#include <cstring>
#include "Mesh.h"
#include "../macros.h"

using namespace L;
using namespace GL;

void MeshBuilder::reset(byte vertexDesc) {
  _vertexDesc = vertexDesc;
  _vertexSize = Mesh::vertexSize(vertexDesc);
  _offsetVertex = Mesh::attributeOffset(_vertexDesc,Mesh::VERTEX);
  _offsetColor = Mesh::attributeOffset(_vertexDesc,Mesh::COLOR);
  _offsetNormal = Mesh::attributeOffset(_vertexDesc,Mesh::NORMAL);
  _indexBuffer.clear();
  _vertexBuffer.clear();
}
void MeshBuilder::computeNormals() {
  L_ASSERT(_vertexDesc & Mesh::NORMAL);
  uint32_t vc(vertexCount());
  for(uint32_t i(0); i<vc; i++)
    normal(i) = Vector3f(0,0,0);
  for(uint32_t i(0); i<_indexBuffer.size(); i+=3) {
    Vector3f& a(vertex(_indexBuffer[i]));
    Vector3f n((vertex(_indexBuffer[i+1])-a)
               .cross(vertex(_indexBuffer[i+2])-a));
    n.normalize();
    normal(_indexBuffer[i]) += n;
    normal(_indexBuffer[i+1]) += n;
    normal(_indexBuffer[i+2]) += n;
  }
  for(uint32_t i(0); i<vc; i++)
    normal(i).normalize();
}
void MeshBuilder::setVertex(const Vector3f& vertex) {
  memcpy(_vertexTmp+_offsetVertex,vertex.bytes(),sizeof(vertex));
}
void MeshBuilder::setVertexColor(const Color& color) {
  memcpy(_vertexTmp+_offsetColor,color.bytes(),sizeof(color));
}
uint32_t MeshBuilder::addVertex() {
  uint32_t wtr(vertexCount());
  _vertexBuffer.size(_vertexBuffer.size()+_vertexSize);
  memcpy(&_vertexBuffer[wtr*_vertexSize],_vertexTmp,_vertexSize);
  return wtr;
}
void MeshBuilder::addIndex(uint32_t index) {
  _indexBuffer.push(index);
}
void MeshBuilder::addTriangle(uint32_t a, uint32_t b, uint32_t c) {
  addIndex(a);
  addIndex(b);
  addIndex(c);
}
