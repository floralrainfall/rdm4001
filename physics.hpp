#pragma once

#include <memory>
#include <mutex>
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include <bullet/BulletCollision/BroadphaseCollision/btBroadphaseInterface.h>
#include <bullet/BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <bullet/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <bullet/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <bullet/btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace rdm {
class World;
class PhysicsWorld {
  std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
  std::unique_ptr<btBroadphaseInterface> overlappingPairCache;
  std::unique_ptr<btCollisionDispatcher> dispatcher;
  std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
  btAlignedObjectArray<std::unique_ptr<btCollisionShape>> collisionShapes;
  std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;
public:
  PhysicsWorld(World* world);

  void stepWorld();

  btDiscreteDynamicsWorld* getWorld() { return dynamicsWorld.get(); }
  std::mutex mutex;
};

class BulletHelpers {
public:
  static btVector3 toVector3(glm::vec3 v) {
    return btVector3(v.x, v.y, v.z);
  };

  static glm::vec3 fromVector3(btVector3 v) {
    return glm::vec3(v.x(), v.y(), v.z());
  }
};
};
