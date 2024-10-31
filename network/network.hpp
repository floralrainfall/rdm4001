#pragma once
#include <enet/enet.h>

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "entity.hpp"
#include "player.hpp"

#define NETWORK_STREAM_META 0
#define NETWORK_STREAM_ENTITY 1

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

  Peer();
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
  size_t ticks;

  std::map<std::string, EntityConstructorFunction> constructors;
  std::map<EntityId, std::unique_ptr<Entity>> entities;

 public:
  NetworkManager(World* world);
  ~NetworkManager();

  World* getWorld() { return world; }

  enum PacketId {
    WelcomePacket,       // S -> C, beginning of handshake
    AuthenticatePacket,  // C -> S
    NewIdPacket,         // S -> C
    DelIdPacket,         // S -> C
    NewPeerPacket,       // S -> C
    DelPeerPacket,       // S -> C
    PeerInfoPacket,      // S -> C
  };

  void service();

  void start(int port = 7938);

  void connect(std::string address, int port = 7938);
  void requestDisconnect();

  void deleteEntity(EntityId id);
  Entity* instantiate(std::string typeName, int id = -1);
  void registerConstructor(EntityConstructorFunction func,
                           std::string typeName);
};
}  // namespace rdm::network
