#include "physics.hpp"

#include <bullet/BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <bullet/BulletCollision/CollisionDispatch/btCollisionConfiguration.h>
#include <bullet/BulletCollision/CollisionDispatch/btCollisionDispatcher.h>

#include "logging.hpp"
#include "scheduler.hpp"
#include "world.hpp"

namespace rdm {
class PhysicsJob : public SchedulerJob {
  PhysicsWorld* world;

 public:
  virtual double getFrameRate() { return PHYSICS_FRAMERATE; }

  PhysicsJob(PhysicsWorld* _world) : SchedulerJob("Physics"), world(_world) {}

  virtual Result step() {
    world->stepWorld();
    return Stepped;
  }
};

PhysicsWorld::PhysicsWorld(World* world) {
  world->getScheduler()->addJob(new PhysicsJob(this));

  collisionConfiguration.reset(new btDefaultCollisionConfiguration());
  dispatcher.reset(new btCollisionDispatcher(collisionConfiguration.get()));
  overlappingPairCache.reset(new btDbvtBroadphase());
  solver.reset(new btSequentialImpulseConstraintSolver);
  dynamicsWorld.reset(
      new btDiscreteDynamicsWorld(dispatcher.get(), overlappingPairCache.get(),
                                  solver.get(), collisionConfiguration.get()));
  dynamicsWorld->setGravity(btVector3(0, -10, 0));

  Log::printf(LOG_DEBUG, "Initialized physics world");
}

void PhysicsWorld::stepWorld() {
  {
    std::scoped_lock l(mutex);
    dynamicsWorld->stepSimulation(PHYSICS_FRAMERATE, 10);
  }
  physicsStepping.fire();
}
};  // namespace rdm
