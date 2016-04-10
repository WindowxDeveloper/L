#ifndef DEF_L_GL_Camera
#define DEF_L_GL_Camera

#include "../math/Interval.h"
#include "../math/Matrix.h"
#include "../math/Vector.h"

namespace L {
  namespace GL {
    class Camera {
      protected:
        Vector3f _position, _lookat, _up, _forward, _right;
        Matrix44f _view, _projection, _viewProjection, _ray;

      public:
        Camera(const Vector3f& position = Vector3f(0,0,1), const Vector3f& lookat = Vector3f(0,0,0));
        void update();

        void move(const Vector3f&); // Changes position of the camera relatively to its orientation
        void position(const Vector3f&); // Directly sets position of the camera
        void lookat(const Vector3f&); // Directly sets the lookat point of the camera

        void phiLook(float); // Rotate camera left/right first person
        void phiPosition(float); // Rotate camera left/right third person
        void thetaLook(float); // Rotate camera down/up first person
        void thetaPosition(float); // Rotate camera down/up third person

        void perspective(float fovy, float aspect, float near, float far); // 3D perspective
        void ortho(float left, float right, float bottom, float top, float near = -1, float far = 1);
        void pixels(); // Maps to window's pixels (origins at top-left pixel)

        bool worldToScreen(const Vector3f&, Vector2f&) const; // Set the screen-space vector for that world space vector, returns false if behind camera
        Vector3f screenToRay(const Vector2f&) const; // Returns direction vector from normalized screen position
        bool sees(const Interval3f&) const; // Checks if an interval can currently be seen by camera

        const Vector3f& position() const {return _position;}
        const Vector3f& up() const {return _up;}
        const Vector3f& forward() const {return _forward;}
        const Vector3f& right() const {return _right;}
        const Matrix44f& view() const {return _view;}
        const Matrix44f& projection() const {return _projection;}
        const Matrix44f& viewProjection() const {return _viewProjection;}
    };
  }
}

#endif




