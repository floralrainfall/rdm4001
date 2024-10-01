#pragma once
#include <glm/glm.hpp>
namespace rdm::gfx {
class Camera {
  bool pdirty, vdirty;

 public:
  enum Projection { Orthographic, Perspective };

  Camera();

  void updateCamera(glm::vec2 framebufferSize);
  void setPosition(glm::vec3 pos) {
    this->eye = pos;
    vdirty = true;
  }
  glm::vec3 getPosition() { return eye; }
  void setTarget(glm::vec3 target) {
    this->target = target;
    vdirty = true;
  }
  glm::vec3 getTarget() { return target; }
  void setProjection(Projection p) {
    this->p = p;
    pdirty = true;
  };
  void setFOV(float fov) {
    this->fov = fov;
    pdirty = true;
  }

  glm::mat4 getProjectionMatrix() { return pmatrix; }
  glm::mat4 getUiProjectionMatrix() { return uipmatrix; }
  glm::mat4 getViewMatrix() { return vmatrix; }
  glm::vec2 getFramebufferSize() { return fbSize; }

 private:
  glm::vec2 fbSize;
  Projection p;
  float fov;
  glm::vec3 eye;
  glm::vec3 target;
  glm::mat4 pmatrix;
  glm::mat4 vmatrix;
  glm::mat4 uipmatrix;
};
}  // namespace rdm::gfx