#pragma once
#include <enet/enet.h>

#include <string>

#include "entity.hpp"

namespace rdm::network {
class Player : public Entity {
 public:
  ReplicateProperty<int> remotePeerId;  // -1 if not controlled, -2 if a bot
  ReplicateProperty<std::string> displayName;

  Player(NetworkManager* manager, EntityId id);

  virtual const char* getTypeName() { return "Player"; };

  bool isLocalPlayer();

  virtual std::string getEntityInfo();

  virtual bool isDirty() { return remotePeerId.isDirty(); }
  virtual bool isBot() { return remotePeerId.get() == -2; }
  virtual void serialize(BitStream& stream);
  virtual void deserialize(BitStream& stream);
};
}  // namespace rdm::network
