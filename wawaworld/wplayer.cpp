#include "wplayer.hpp"

#include "gfx/engine.hpp"
#include "putil/fpscontroller.hpp"
#include "world.hpp"
namespace ww {
WPlayer::WPlayer(net::NetworkManager* manager, net::EntityId id)
    : Player(manager, id) {
  controller.reset(
      new rdm::putil::FpsController(manager->getWorld()->getPhysicsWorld()));
}

void WPlayer::tick() {
  if (!getManager()->isBackend()) {
    if (getManager()->getLocalPeer().peerId == remotePeerId.get()) {
      controller->updateCamera(getGfxEngine()->getCamera());
      controller->setLocalPlayer(true);

      btTransform transform;
      controller->getMotionState()->getWorldTransform(transform);
      getManager()->addPendingUpdateUnreliable(getEntityId());
    } else {
      controller->setLocalPlayer(false);
    }
  }
}

void WPlayer::serialize(net::BitStream& stream) { Player::serialize(stream); }

void WPlayer::deserialize(net::BitStream& stream) {
  Player::deserialize(stream);
}

void WPlayer::serializeUnreliable(net::BitStream& stream) {
  btTransform transform;
  controller->getMotionState()->getWorldTransform(transform);
  stream.write<btVector3>(transform.getOrigin());
  stream.write<btMatrix3x3>(transform.getBasis());
}

void WPlayer::deserializeUnreliable(net::BitStream& stream) {
  btTransform transform;
  transform.setOrigin(stream.read<btVector3>());
  transform.setBasis(stream.read<btMatrix3x3>());

  if (!getManager()->isBackend()) {
    if (getManager()->getLocalPeer().peerId == remotePeerId.get()) {
    } else {
      controller->getMotionState()->setWorldTransform(transform);
    }
  }
}
}  // namespace ww
