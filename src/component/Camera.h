#pragma once

#include "Transform.h"
#include "../engine/Resource.h"
#include "../rendering/GPUBuffer.h"
#include "../rendering/Framebuffer.h"
#include "../rendering/Material.h"
#include "../math/Interval.h"
#include "../time/Time.h"

namespace L {
  class Camera : public TComponent<Camera, ComponentFlag::WindowEvent> {
  protected:
    Transform* _transform;
    Matrix44f _view, _projection, _viewProjection, _prevViewProjection, _ray;
    Interval2f _viewport;
    enum {
      Perspective,
      Ortho
    } _projectionType;
    float _fovy, _near, _far, _left, _right, _bottom, _top;
    Material _present_material;
    VkCommandBuffer _cmd_buffer;
    VkViewport _vk_viewport;
    Framebuffer _geometry_buffer, _light_buffer;
    Time _framebuffer_mtime;
    GPUBuffer _shared_uniform;
  public:
    Camera();
    inline Camera(const Camera&) : Camera() { error("Camera component should not be copied."); }
    inline Camera& operator=(const Camera&) { error("Camera component should not be copied."); return *this; }

    virtual void update_components() override;
    virtual Map<Symbol, Var> pack() const override;
    virtual void unpack(const Map<Symbol, Var>&) override;
    static void script_registration();

    void resize_buffers();
    void event(const Window::Event&);
    void prerender(VkCommandBuffer);
    void present();

    void viewport(const Interval2f& viewport);
    void perspective(float fovy, float near, float far); // 3D perspective
    void ortho(float left, float right, float bottom, float top, float near = -1, float far = 1);
    void pixels(); // Maps to window's pixels (origins at top-left pixel)
    void update_projection();
    void update_viewport();

    bool worldToScreen(const Vector3f&, Vector2f&) const; // Finds the normalized screen position for that world space vector, returns false if behind camera
    Vector3f screenToRay(const Vector2f&) const; // Returns direction vector from normalized screen position
    Vector2f screenToPixel(const Vector2f&) const; // Returns pixel position from NDC
    Interval2i viewportPixel() const;
    bool sees(const Interval3f&) const; // Checks if an interval can currently be seen by camera
    void frustum_planes(Vector4f[6]) const; // In world-space

    inline const Matrix44f& view() const { return _view; }
    inline const Matrix44f& projection() const { return _projection; }
    inline const Matrix44f& viewProjection() const { return _viewProjection; }
    inline Material& present_material() { return _present_material; }
    inline Framebuffer& geometry_buffer() { return _geometry_buffer; }
    inline const Framebuffer& geometry_buffer() const { return _geometry_buffer; }
    inline Framebuffer& light_buffer() { return _light_buffer; }
    inline const Framebuffer& light_buffer() const { return _light_buffer; }
    inline const GPUBuffer& shared_uniform() const { return _shared_uniform; }
    inline VkCommandBuffer cmd_buffer() const { return _cmd_buffer; }
    inline Time framebuffer_mtime() const { return _framebuffer_mtime; }
  };
}
