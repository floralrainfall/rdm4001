#include "camera.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace rdm::gfx {
Camera::Camera() {
  eye = glm::vec3(2, 2, 2);
  target = glm::vec3(0, 2, 0);
  up = glm::vec3(0, 0, 1);
  fov = 90.f;
  p = Perspective;
  leftHanded = false;
  pdirty = true;
  vdirty = true;
}

void Camera::updateCamera(glm::vec2 framebufferSize) {
  if (pdirty || fbSize != framebufferSize) {
    switch (p) {
      case Perspective:
        pmatrix = glm::perspective(fov, framebufferSize.x / framebufferSize.y,
                                   1.0f, 65535.f);
        break;
      case Orthographic:
        break;
    }
    uipmatrix = glm::ortho(0.f, framebufferSize.x, 0.f, framebufferSize.y);
    fbSize = framebufferSize;
    pdirty = false;
  }
  if (vdirty) {
    if(leftHanded)
      vmatrix = glm::lookAtLH(target, eye, up);
    else
      vmatrix = glm::lookAt(target, eye, up);
    vdirty = false;
  }
}
}  // namespace rdm::gfx
