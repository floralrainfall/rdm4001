#pragma once
#include "gfx/camera.hpp"
#include "physics.hpp"
namespace rdm::putil {
struct FpsControllerSettings {
  float capsuleHeight;
  float capsuleRadius;
  float capsuleMass;
  float maxSpeed;
  bool enabled;

  FpsControllerSettings();  // default settings, good for bsp maps
};

class FpsController {
  PhysicsWorld* world;
  std::unique_ptr<btRigidBody> rigidBody;
  std::unique_ptr<btMotionState> motionState;
  FpsControllerSettings settings;

  float cameraPitch;
  float cameraYaw;
  glm::mat3 cameraView;
  glm::mat3 moveView;
  glm::vec2 moveVel;

  void physicsStep();

 public:
  FpsController(PhysicsWorld* world,
                FpsControllerSettings settings = FpsControllerSettings());
  ~FpsController();

  void updateCamera(gfx::Camera& camera);

  btRigidBody* getRigidBody() { return rigidBody.get(); };
  btMotionState* getMotionState() { return motionState.get(); };
  FpsControllerSettings& getSettings() { return settings; }
};
};  // namespace rdm::putil
