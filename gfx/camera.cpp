#include "camera.hpp"

#include <cmath>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace rdm::gfx {
Camera::Camera() {
  eye = glm::vec3(2, 2, 2);
  target = glm::vec3(0, 2, 0);
  up = glm::vec3(0, 0, 1);
  fov = 90.f;
  near = 1.f;
  far = 65535.f;
  p = Perspective;
  leftHanded = false;
  pdirty = true;
  vdirty = true;
}

void Camera::updateCamera(glm::vec2 framebufferSize) {
  if (pdirty || fbSize != framebufferSize) {
    switch (p) {
      case Perspective:
        pmatrix =
            glm::perspective(fov * (M_PIf / 180.f),
                             framebufferSize.x / framebufferSize.y, near, far);
        break;
      case Orthographic:
        pmatrix = glm::ortho(0.f, 1.f, 0.f, 1.f);
        break;
    }
    uipmatrix = glm::ortho(0.f, framebufferSize.x, 0.f, framebufferSize.y);
    fbSize = framebufferSize;
    pdirty = false;
  }
  if (vdirty) {
    if (leftHanded)
      vmatrix = glm::lookAtLH(eye, target, up);
    else
      vmatrix = glm::lookAt(eye, target, up);
    vdirty = false;
  }
}
}  // namespace rdm::gfx
