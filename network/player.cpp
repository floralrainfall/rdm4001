#include "player.hpp"
namespace rdm::network {
Player::Player(NetworkManager* manager, EntityId id) : Entity(manager, id) {}

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
