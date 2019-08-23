#include "Mesh.h"

#include "MeshBuilder.h"

using namespace L;

Mesh::Mesh(size_t count, const void* data, size_t size, const VkFormat* formats, size_t fcount, const uint16_t* iarray, size_t icount)
  : Mesh() {
  load(count, data, size, formats, fcount, iarray, icount);
}
Mesh::Mesh(const MeshBuilder& mb, const VkFormat* formats, size_t fcount)
  : Mesh() {
  load(mb, formats, fcount);
}
Mesh::~Mesh() {
  if(_vertex_buffer) Memory::delete_type(_vertex_buffer);
  if(_index_buffer) Memory::delete_type(_index_buffer);
}
void Mesh::load(size_t count, const void* data, size_t size, const VkFormat* formats, size_t fcount, const uint16_t* iarray, size_t icount) {
  if(_vertex_buffer) {
    Memory::delete_type(_vertex_buffer);
    _vertex_buffer = nullptr;
  }
  if(_index_buffer) {
    Memory::delete_type(_index_buffer);
    _index_buffer = nullptr;
  }
  _formats = Array<VkFormat>(formats, fcount);

  _vertex_buffer = Memory::new_type<GPUBuffer>(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  _vertex_buffer->load(data);
  if(iarray) {
    _index_buffer = Memory::new_type<GPUBuffer>(icount*sizeof(*iarray), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    _index_buffer->load(iarray);
    _count = uint32_t(icount);
  } else {
    _count = uint32_t(count);
  }

  // Compute bounds
  const size_t vertex_size(size/count);
  _bounds = (*(Vector3f*)data);
  for(uintptr_t i(1); i<count; i++)
    _bounds.add(*(Vector3f*)((uint8_t*)data+vertex_size*i));
}
void Mesh::load(const MeshBuilder& mb, const VkFormat* formats, size_t fcount) {
  load(mb.vertex_count(), mb.vertices(), mb.vertices_size(), formats, fcount, mb.indices(), mb.index_count());
}
void Mesh::draw(VkCommandBuffer cmd_buffer) const {
  if(_vertex_buffer) {
    const VkBuffer buffers[] {*_vertex_buffer};
    const VkDeviceSize offsets[] {0};
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, buffers, offsets);
    if(_index_buffer) {
      vkCmdBindIndexBuffer(cmd_buffer, *_index_buffer, 0, VK_INDEX_TYPE_UINT16);
      vkCmdDrawIndexed(cmd_buffer, _count, 1, 0, 0, 0);
    } else {
      vkCmdDraw(cmd_buffer, _count, 1, 0, 0);
    }
  }
}

const Mesh& Mesh::quad() {
  static const float quad[] = {
    -1,-1,0,
    1,-1,0,
    -1,1,0,
    1,1,0,
  };
  static const VkFormat formats[] {VK_FORMAT_R32G32B32_SFLOAT};
  static Mesh mesh(4, quad, sizeof(quad), formats, L_COUNT_OF(formats));
  return mesh;
}
const Mesh& Mesh::wire_cube() {
  static const float wireCube[] = {
    // Bottom face
    -1,-1,-1, -1,1,-1,
    -1,-1,-1, 1,-1,-1,
    1,-1,-1,  1,1,-1,
    -1,1,-1,  1,1,-1,
    // Top face
    -1,-1,1, -1,1,1,
    -1,-1,1, 1,-1,1,
    1,-1,1,  1,1,1,
    -1,1,1,  1,1,1,
    // Sides
    -1,-1,-1, -1,-1,1,
    -1,1,-1,  -1,1,1,
    1,-1,-1,  1,-1,1,
    1,1,-1,   1,1,1,
  };
  static const VkFormat formats[] {VK_FORMAT_R32G32B32_SFLOAT};
  static Mesh mesh(12*2, wireCube, sizeof(wireCube), formats, L_COUNT_OF(formats));
  return mesh;
}
const Mesh& Mesh::wire_sphere() {
  static const float d(sqrt(.5f));
  static const float wireSphere[] = {
    // X circle
    0,0,-1, 0,-d,-d, 0,-d,-d, 0,-1,0,
    0,-1,0, 0,-d,d,  0,-d,d,  0,0,1,
    0,0,1,  0,d,d,   0,d,d,   0,1,0,
    0,1,0,  0,d,-d,  0,d,-d,  0,0,-1,
    // Y circle
    0,0,-1, -d,0,-d, -d,0,-d, -1,0,0,
    -1,0,0, -d,0,d,  -d,0,d,  0,0,1,
    0,0,1,  d,0,d,   d,0,d,   1,0,0,
    1,0,0,  d,0,-d,  d,0,-d,  0,0,-1,
    // Z circle
    0,-1,0, -d,-d,0, -d,-d,0, -1,0,0,
    -1,0,0, -d,d,0,  -d,d,0,  0,1,0,
    0,1,0,  d,d,0,   d,d,0,   1,0,0,
    1,0,0, d,-d,0,   d,-d,0,  0,-1,0,
  };
  static const VkFormat formats[] {VK_FORMAT_R32G32B32_SFLOAT};
  static Mesh mesh(24*2, wireSphere, sizeof(wireSphere), formats, L_COUNT_OF(formats));
  return mesh;
}
