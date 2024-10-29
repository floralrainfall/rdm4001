#pragma once
#include <enet/enet.h>

#include <string>

#include "entity.hpp"

namespace rdm::network {
class Player : public Entity {
  ReplicateProperty<int> remotePeerId;  // -1 if not controlled
  ReplicateProperty<std::string> displayName;

 public:
  Player(NetworkManager* manager, EntityId id);

  virtual const char* getTypeName() { return "Player"; };
  int getRemotePeerId() { return remotePeerId.get(); }
};
}  // namespace rdm::network
