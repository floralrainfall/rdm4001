#pragma once
#include "gfx/camera.hpp"
#include "network/bitstream.hpp"
#include "physics.hpp"
namespace rdm::putil {
struct FpsControllerSettings {
  float capsuleHeight;
  float capsuleRadius;
  float capsuleMass;
  float maxSpeed;
  float maxAccel;
  float stopSpeed;
  float friction;
  bool enabled;

  FpsControllerSettings();  // default settings, good for bsp maps
};

class FpsController {
  PhysicsWorld* world;
  std::unique_ptr<btRigidBody> rigidBody;
  btMotionState* motionState;
  FpsControllerSettings settings;
  bool localPlayer;

  ClosureId stepJob;

  float cameraPitch;
  float cameraYaw;
  glm::mat3 cameraView;
  glm::mat3 moveView;
  glm::vec2 moveVel;
  glm::vec2 accel;
  glm::vec3 networkPosition;
  bool grounded;

  void physicsStep();

  void moveGround(btVector3& vel, glm::vec2 wishdir);
  void moveAir(btVector3& vel, glm::vec2 wishdir);

 public:
  FpsController(PhysicsWorld* world,
                FpsControllerSettings settings = FpsControllerSettings());
  ~FpsController();

  void setLocalPlayer(bool b) { localPlayer = b; };
  void updateCamera(gfx::Camera& camera);

  void serialize(network::BitStream& stream);
  void deserialize(network::BitStream& stream);

  glm::vec3 getNetworkPosition() { return networkPosition; }
  glm::vec2 getWishDir() { return accel; }

  btTransform getTransform() { return rigidBody->getWorldTransform(); }
  bool isGrounded() { return grounded; }

  btRigidBody* getRigidBody() { return rigidBody.get(); };
  btMotionState* getMotionState() { return motionState; };
  FpsControllerSettings& getSettings() { return settings; }
};
};  // namespace rdm::putil
