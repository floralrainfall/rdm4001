#include "fpscontroller.hpp"

#include <algorithm>
#include <glm/common.hpp>
#include <glm/glm.hpp>

#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "input.hpp"
#include "logging.hpp"
#include "physics.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#ifndef DISABLE_EASY_PROFILER
#include <easy/profiler.h>
#endif

namespace rdm::putil {
FpsControllerSettings::FpsControllerSettings() {
  capsuleHeight = 46.f;
  capsuleRadius = 16.f;
  capsuleMass = 1.f;
  maxSpeed = 160.f;
  maxAccel = 10.f;
  friction = 4.0f;
  stopSpeed = 40.f;
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
    cameraPitch -= mouseDelta.x * (M_PI / 180.0);
    cameraYaw -= mouseDelta.y * (M_PI / 180.0);
  }

  glm::quat yawQuat = glm::angleAxis(cameraYaw, glm::vec3(0.f, 1.f, 0.f));
  glm::quat pitchQuat = glm::angleAxis(cameraPitch, glm::vec3(0.f, 0.f, 1.f));
  cameraView = glm::toMat3(pitchQuat * yawQuat);
  moveView = glm::toMat3(pitchQuat);

  glm::vec3 forward = glm::vec3(1, 0, 0);
  camera.setPosition(origin);
  camera.setTarget(origin + (cameraView * forward));
}

void FpsController::physicsStep() {
#ifndef DISABLE_EASY_PROFILER
  EASY_FUNCTION("FpsController::physicsStep");
#endif

  btTransform transform = rigidBody->getWorldTransform();
  btVector3 start =
      transform.getOrigin() + btVector3(0, 0, -settings.capsuleHeight / 2.0);
  btVector3 end = start + btVector3(0, 0, -17);
  btDynamicsWorld::ClosestRayResultCallback callback(start, end);
  world->getWorld()->rayTest(start, end, callback);

  grounded = (callback.m_collisionObject != NULL);

  if (localPlayer) {
#ifndef DISABLE_EASY_PROFILER
    EASY_BLOCK("Local Player Calculation");
#endif

    // Log::printf(LOG_DEBUG, "%.2f, %.2f, %.2f", transform.getOrigin().x(),
    // transform.getOrigin().y(), transform.getOrigin().z());

    Input::Axis* fbA = Input::singleton()->getAxis("ForwardBackward");
    Input::Axis* lrA = Input::singleton()->getAxis("LeftRight");

    glm::vec2 wishdir =
        glm::vec2(moveView * glm::vec3(-fbA->value, -lrA->value, 0.0));
    accel = wishdir;

    btVector3 vel = rigidBody->getLinearVelocity();

    // modelled after Quake 1 movement
    if (grounded) {
      // apply ground friction

      float speed = vel.length();
      float control = speed < settings.stopSpeed ? settings.stopSpeed : speed;
      float newspeed = speed - PHYSICS_FRAMERATE * settings.friction * control;

      if (newspeed >= 0) {
        newspeed /= speed;

        vel *= newspeed;
      }

      if (Input::singleton()->isKeyDown(' ')) {
        vel += btVector3(0, 0, settings.maxSpeed);
      }
    }

    float currentSpeed = vel.dot(btVector3(wishdir.x, wishdir.y, 0.0));
    float addSpeed =
        std::clamp(settings.maxSpeed - currentSpeed, 0.f, settings.maxAccel);
    vel += addSpeed * btVector3(wishdir.x, wishdir.y, 0.0);

    rigidBody->setLinearVelocity(vel);
    btTransform& transform = rigidBody->getWorldTransform();
    transform.setBasis(BulletHelpers::toMat3(moveView));
    rigidBody->setWorldTransform(transform);
  }
}

void FpsController::serialize(network::BitStream& stream) {
  btTransform transform;
  getMotionState()->getWorldTransform(transform);
  btVector3FloatData vectorData;
  transform.getOrigin().serialize(vectorData);
  stream.write<btVector3FloatData>(vectorData);
  btMatrix3x3FloatData matrixData;
  transform.getBasis().serialize(matrixData);
  stream.write<btMatrix3x3FloatData>(matrixData);
}

void FpsController::deserialize(network::BitStream& stream) {
  btVector3 origin;
  origin.deSerialize(stream.read<btVector3FloatData>());

  btMatrix3x3 basis;
  basis.deSerialize(stream.read<btMatrix3x3FloatData>());

  if (!localPlayer) {
    btTransform& bodyTransform = rigidBody->getWorldTransform();
    bodyTransform.setOrigin(origin);
    bodyTransform.setBasis(basis);
    rigidBody->setWorldTransform(bodyTransform);
  }
}

};  // namespace rdm::putil
