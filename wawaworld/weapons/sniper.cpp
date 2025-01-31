#include "sniper.hpp"

#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

#include "game.hpp"
#include "logging.hpp"
#include "network/entity.hpp"
#include "network/network.hpp"
#include "physics.hpp"
#include "sound.hpp"
#include "wplayer.hpp"

#define SHOT_DISTANCE 65535.0

namespace ww {
WeaponSniper::WeaponSniper(net::NetworkManager* manager, net::EntityId id)
    : Weapon(manager, id) {
  viewModel = "dat5/weapons/w_sniper_rifle.glb";
  worldModel = "dat5/weapons/w_sniper_rifle.glb";

  if (!manager->isBackend()) {
    emitter.reset(manager->getGame()->getSoundManager()->newEmitter());
  }
}

void WeaponSniper::tick() {
  if (!getManager()->isBackend() && getOwnerRef()) {
    emitter->node = getOwnerRef()->getNode();
  }
}

void WeaponSniper::primaryFire() {
  if (nextPrimary > getManager()->getDistributedTime()) return;
  rdm::Log::printf(rdm::LOG_DEBUG, "primaryFire");

  if (getManager()->isBackend()) {
  } else {
    emitter->play(getManager()
                      ->getGame()
                      ->getSoundManager()
                      ->getSoundCache()
                      ->get("dat5/weapons/357_shot1.wav")
                      .value());
  }

  btTransform ownerTransform = getOwnerRef()->getController()->getTransform();
  btVector3 front = getOwnerRef()->getController()->getFront();
  btVector3 from = ownerTransform.getOrigin();
  btVector3 to =
      from + (getOwnerRef()->getController()->getFront() * SHOT_DISTANCE);
  btCollisionWorld::AllHitsRayResultCallback callback(from, to);
  getWorld()->getPhysicsWorld()->getWorld()->rayTest(from, to, callback);
  rdm::Log::printf(rdm::LOG_DEBUG, "Hit %i, travelled %f%%",
                   callback.m_collisionObjects.size(),
                   callback.m_closestHitFraction * 100.0);
  rdm::Log::printf(rdm::LOG_DEBUG, "N: %f, %f, %f", front.x(), front.y(),
                   front.z());
  rdm::Log::printf(rdm::LOG_DEBUG, "-> %f, %f, %f", to.x(), to.y(), to.z());
  for (int i = 0; i < callback.m_collisionObjects.size(); i++) {
    bool hitSomething = false;
    btVector3 hitWorld = callback.m_hitPointWorld[i];
    btVector3 hitNormal = callback.m_hitNormalWorld[i];
    float hitRatio = callback.m_hitFractions[i];
    if (callback.m_collisionObjects[i]->getUserIndex() & PHYSICS_INDEX_PLAYER) {
      WPlayer* player =
          (WPlayer*)callback.m_collisionObjects[i]->getUserPointer();
      rdm::Log::printf(rdm::LOG_DEBUG, "Hit player %p", player);
      hitSomething = true;
    }
    if (callback.m_collisionObjects[i]->getUserIndex() & PHYSICS_INDEX_WORLD) {
      rdm::Log::printf(rdm::LOG_DEBUG, "Hit world");
      hitSomething = true;
    }
    if (hitSomething) {
      rdm::Log::printf(rdm::LOG_DEBUG, "Hit at %f, %f, %f", hitWorld.x(),
                       hitWorld.y(), hitWorld.z());
      rdm::Log::printf(rdm::LOG_DEBUG, "Hit norm %f, %f, %f", hitNormal.x(),
                       hitNormal.y(), hitNormal.z());
      rdm::Log::printf(rdm::LOG_DEBUG, "Distance: %f",
                       hitRatio * SHOT_DISTANCE);
      break;
    }
  }

  nextPrimary = getManager()->getDistributedTime() + 0.f;
}
}  // namespace ww
