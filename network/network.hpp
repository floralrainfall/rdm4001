#pragma once
#include <enet/enet.h>

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "entity.hpp"
#include "player.hpp"

namespace rdm {
class World;
}

namespace rdm::network {
struct Peer {
  enum Type {
    ConnectedPlayer,
    Machine,
    Unconnected,
    Undifferentiated,
  };

  ENetPeer* peer;
  int peerId;
  Player* playerEntity;
  Type type;

  std::vector<EntityId> pendingNewIds;
  std::vector<EntityId> pendingDelIds;
};

typedef std::function<Entity*(NetworkManager*, EntityId)>
    EntityConstructorFunction;

class NetworkManager {
  friend class NetworkJob;

  ENetHost* host;
  Peer localPeer;
  std::map<int, Peer> peers;
  World* world;
  bool backend;

  EntityId lastId;
  int lastPeerId;

  std::map<std::string, EntityConstructorFunction> constructors;
  std::map<EntityId, std::unique_ptr<Entity>> entities;

  Entity* receiveEntity(std::string typeName, EntityId id);

 public:
  NetworkManager(World* world);
  ~NetworkManager();

  void service();

  void start(int port = 7938);

  void connect(std::string address, int port = 7938);
  void requestDisconnect();

  Entity* instantiate(std::string typeName);
  void registerConstructor(EntityConstructorFunction func,
                           std::string typeName);
};
}  // namespace rdm::network
