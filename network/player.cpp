#include "player.hpp"

#include "network.hpp"
namespace rdm::network {
Player::Player(NetworkManager* manager, EntityId id) : Entity(manager, id) {
  remotePeerId.set(-1);
}

bool Player::isLocalPlayer() {
  if (getManager()->isBackend())
    throw std::runtime_error("Calling isLocalPlayer on backend");
  return getManager()->getLocalPeer().playerEntity == this;
}

void Player::serialize(BitStream& stream) {
  Entity::serialize(stream);
  remotePeerId.serialize(stream);
  displayName.serialize(stream);
}

void Player::deserialize(BitStream& stream) {
  Entity::deserialize(stream);
  remotePeerId.deserialize(stream);
  displayName.deserialize(stream);
}
}  // namespace rdm::network
