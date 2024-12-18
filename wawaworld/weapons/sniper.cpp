#include "sniper.hpp"

#include "game.hpp"
#include "network/entity.hpp"
#include "network/network.hpp"
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

  nextPrimary = getManager()->getDistributedTime() + 5.f;
}
}  // namespace ww
