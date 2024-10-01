#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
namespace rdm::gfx {
Camera::Camera() {
  eye = glm::vec3(2, 2, 2);
  target = glm::vec3(0, 2, 0);
  fov = 90.f;
  p = Perspective;
  pdirty = true;
  vdirty = true;
}

void Camera::updateCamera(glm::vec2 framebufferSize) {
  if (pdirty || fbSize != framebufferSize) {
    switch (p) {
      case Perspective:
        pmatrix = glm::perspective(fov, framebufferSize.x / framebufferSize.y,
                                   0.1f, 1000.f);
        break;
      case Orthographic:
        break;
    }
    uipmatrix = glm::ortho(0.f, framebufferSize.x, 0.f, framebufferSize.y);
    fbSize = framebufferSize;
    pdirty = false;
  }
  if (vdirty) {
    vmatrix = glm::lookAt(eye, target, glm::vec3(0, 0, 1));
    vdirty = false;
  }
}
}  // namespace rdm::gfx