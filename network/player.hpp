#pragma once
#include <enet/enet.h>

#include <string>

#include "entity.hpp"

namespace rdm::network {
class Player : public Entity {
 public:
  ReplicateProperty<int> remotePeerId;  // -1 if not controlled
  ReplicateProperty<std::string> displayName;

  Player(NetworkManager* manager, EntityId id);

  virtual const char* getTypeName() { return "Player"; };

  bool isLocalPlayer();

  virtual bool isDirty() { return remotePeerId.isDirty(); }
  virtual void serialize(BitStream& stream);
  virtual void deserialize(BitStream& stream);
};
}  // namespace rdm::network
