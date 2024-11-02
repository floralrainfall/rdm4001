#include "wplayer.hpp"

#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btVector3.h"
#include "gfx/engine.hpp"
#include "logging.hpp"
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
  std::scoped_lock lock(getWorld()->getPhysicsWorld()->mutex);

  btTransform transform;
  controller->getMotionState()->getWorldTransform(transform);
  btVector3FloatData vectorData;
  transform.getOrigin().serialize(vectorData);
  stream.write<btVector3FloatData>(vectorData);
  btMatrix3x3FloatData matrixData;
  transform.getBasis().serialize(matrixData);
  stream.write<btMatrix3x3FloatData>(matrixData);
}

void WPlayer::deserializeUnreliable(net::BitStream& stream) {
  std::scoped_lock lock(getWorld()->getPhysicsWorld()->mutex);
  btTransform transform;
  // transform.setOrigin(stream.read<btVector3>());
  // transform.setBasis(stream.read<btMatrix3x3>());

  btVector3 origin;
  origin.deSerialize(stream.read<btVector3FloatData>());
  transform.setOrigin(origin);

  btMatrix3x3 basis;
  basis.deSerialize(stream.read<btMatrix3x3FloatData>());
  transform.setBasis(basis);

  rdm::Log::printf(rdm::LOG_DEBUG, "%f, %f, %f", transform.getOrigin().x(),
                   transform.getOrigin().y(), transform.getOrigin().z());

  if (!getManager()->isBackend()) {
    if (getManager()->getLocalPeer().peerId == remotePeerId.get()) {
    } else {
      controller->getMotionState()->setWorldTransform(transform);
    }
  }
}
}  // namespace ww
