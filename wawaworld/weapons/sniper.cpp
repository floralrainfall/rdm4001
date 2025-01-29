#include "sniper.hpp"

#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

#include "game.hpp"
#include "logging.hpp"
#include "network/entity.hpp"
#include "network/network.hpp"
#include "physics.hpp"
#include "sound.hpp"
#include "wplayer.hpp"
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
  btVector3 from = ownerTransform.getOrigin();
  btVector3 to = from + (btVector3(1, 0, 0)) * 100.0;
  btCollisionWorld::AllHitsRayResultCallback callback(from, to);
  getWorld()->getPhysicsWorld()->getWorld()->rayTest(from, to, callback);
  rdm::Log::printf(rdm::LOG_DEBUG, "Hit %i, travelled %f%%",
                   callback.m_collisionObjects.size(),
                   callback.m_closestHitFraction * 100.0);
  for (int i = 0; i < callback.m_collisionObjects.size(); i++) {
    if (callback.m_collisionObjects[i]->getUserIndex() & PHYSICS_INDEX_PLAYER) {
      WPlayer* player =
          (WPlayer*)callback.m_collisionObjects[i]->getUserPointer();
      rdm::Log::printf(rdm::LOG_DEBUG, "Hit player %p", player);
    }
    if (callback.m_collisionObjects[i]->getUserIndex() & PHYSICS_INDEX_WORLD) {
      rdm::Log::printf(rdm::LOG_DEBUG, "Hit world");
    }
  }

  nextPrimary = getManager()->getDistributedTime() + 5.f;
}
}  // namespace ww
