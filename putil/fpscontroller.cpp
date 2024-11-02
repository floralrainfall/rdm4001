#include "fpscontroller.hpp"

#include <glm/glm.hpp>

#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "input.hpp"
#include "logging.hpp"
#include "physics.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace rdm::putil {
FpsControllerSettings::FpsControllerSettings() {
  capsuleHeight = 46.f;
  capsuleRadius = 16.f;
  capsuleMass = 1.f;
  maxSpeed = 160.f;
}

FpsController::FpsController(PhysicsWorld* world,
                             FpsControllerSettings settings) {
  this->world = world;
  this->settings = settings;
  btCapsuleShape* shape =
      new btCapsuleShapeZ(settings.capsuleRadius, settings.capsuleHeight);
  btTransform transform = btTransform::getIdentity();
  transform.setOrigin(btVector3(-30.0, 30.0, 200.0));
  motionState = new btDefaultMotionState(transform);
  btVector3 inertia;
  shape->calculateLocalInertia(settings.capsuleMass, inertia);
  btRigidBody::btRigidBodyConstructionInfo rbInfo(settings.capsuleMass,
                                                  motionState, shape, inertia);
  rigidBody.reset(new btRigidBody(rbInfo));
  {
    std::scoped_lock l(world->mutex);
    world->getWorld()->addRigidBody(rigidBody.get());
    stepJob = world->physicsStepping.listen([this] { physicsStep(); });
  }

  cameraPitch = 0.f;
  cameraYaw = 0.f;

  localPlayer = false;

  moveVel = glm::vec2(0.0);

  rigidBody->setAngularFactor(btVector3(0, 0, 1));
}

FpsController::~FpsController() {
  world->physicsStepping.removeListener(stepJob);
}

void FpsController::updateCamera(gfx::Camera& camera) {
  btTransform transform;
  motionState->getWorldTransform(transform);

  glm::vec3 origin =
      BulletHelpers::fromVector3(transform.getOrigin()) + glm::vec3(0, 0, 17);

  if (localPlayer) {
    glm::vec2 mouseDelta = Input::singleton()->getMouseDelta();
    cameraPitch += mouseDelta.x * (M_PI / 180.0);
    cameraYaw += mouseDelta.y * (M_PI / 180.0);
  }

  glm::quat yawQuat = glm::angleAxis(cameraYaw, glm::vec3(0.f, -1.f, 0.f));
  glm::quat pitchQuat = glm::angleAxis(cameraPitch, glm::vec3(0.f, 0.f, -1.f));
  cameraView = glm::toMat3(pitchQuat * yawQuat);
  moveView = glm::toMat3(pitchQuat);

  glm::vec3 forward = glm::vec3(-1, 0, 0);
  camera.setPosition(origin);
  camera.setTarget(origin + (cameraView * forward));
}

void FpsController::physicsStep() {
  btTransform transform = rigidBody->getWorldTransform();
  btVector3 start =
      transform.getOrigin() + btVector3(0, 0, -settings.capsuleHeight / 2.0);
  btVector3 end = start + btVector3(0, 0, -16);
  btDynamicsWorld::ClosestRayResultCallback callback(start, end);
  world->getWorld()->rayTest(start, end, callback);

  grounded = (callback.m_collisionObject != NULL);

  if (localPlayer) {
    Input::Axis* fbA = Input::singleton()->getAxis("ForwardBackward");
    Input::Axis* lrA = Input::singleton()->getAxis("LeftRight");

    glm::vec2 wishdir =
        glm::vec2(moveView * glm::vec3(fbA->value, lrA->value, 0.0));
    glm::vec3 move = glm::vec3();

    accel = wishdir;

    btVector3 vel = rigidBody->getLinearVelocity();
    vel += BulletHelpers::toVector3(glm::vec3(accel, 0.0)) * PHYSICS_FRAMERATE *
           settings.maxSpeed;
    rigidBody->setLinearVelocity(vel);
  }
}

};  // namespace rdm::putil
