#include "network.hpp"

#include <enet/enet.h>

#include <stdexcept>

#include "logging.hpp"
#include "network/entity.hpp"
#include "scheduler.hpp"
#include "world.hpp"

static const char* disconnectReasons[] = {"Disconnect by engine",
                                          "Disconnect by user", "Timeout"};

namespace rdm::network {
Peer::Peer() {
  playerEntity = NULL;
  peer = NULL;
}

class NetworkJob : public SchedulerJob {
  NetworkManager* netmanager;

 public:
  NetworkJob(NetworkManager* netmanager)
      : SchedulerJob("Network"), netmanager(netmanager) {}

  virtual Result step() {
    netmanager->service();
    return Stepped;
  }
};

NetworkManager::NetworkManager(World* world) {
  enet_initialize();
  host = NULL;
  this->world = world;
  world->getScheduler()->addJob(new NetworkJob(this));

  lastId = 0;
  lastPeerId = 0;
  ticks = 0;

  registerConstructor(EntityConstructor<Player>, "Player");
  playerType = "Player";
}

NetworkManager::~NetworkManager() {
  if (host) {
    ENetEvent event;
    if (backend) {
      BitStream shutdownMessage;
      shutdownMessage.write<PacketId>(DisconnectPacket);
      shutdownMessage.writeString("Server is shutting down");
      enet_host_broadcast(
          host, 0, shutdownMessage.createPacket(ENET_PACKET_FLAG_RELIABLE));
      enet_host_service(host, &event,
                        1);  // service to flush disconnect packet
    } else {
      if (localPeer.type != Peer::Unconnected) {
        enet_peer_disconnect(localPeer.peer, NETWORK_DISCONNECT_FORCED);
        enet_host_service(host, &event,
                          1);  // service to flush disconnect packet
      }
    }
    enet_host_destroy(host);
  }
  enet_deinitialize();
}

void NetworkManager::service() {
  if (!host) return;

  ENetEvent event;
  while (enet_host_service(host, &event, 1) > 0) {
    switch (event.type) {
      case ENET_EVENT_TYPE_RECEIVE: {
        try {
          Peer* remotePeer = (Peer*)event.peer->data;
          BitStream stream(event.packet->data, event.packet->dataLength);
          PacketId packetId = stream.read<PacketId>();
          switch (packetId) {
            case WelcomePacket:
              if (backend) {
                throw std::runtime_error("Received WelcomePacket on backend");
              } else {
                Log::printf(LOG_INFO, "Received WelcomePacket");
                localPeer.peerId = stream.read<int>();
                size_t _ticks = stream.read<size_t>();
                Log::printf(LOG_DEBUG, "Tick diff %i vs %i (%i)", ticks, _ticks,
                            _ticks - ticks);
                ticks = _ticks;

                BitStream authenticateStream;
                authenticateStream.write<PacketId>(AuthenticatePacket);
                authenticateStream.writeString("Username");
                enet_peer_send(
                    localPeer.peer, NETWORK_STREAM_META,
                    authenticateStream.createPacket(ENET_PACKET_FLAG_RELIABLE));
              }
              break;
            case AuthenticatePacket:
              if (backend) {
                std::string username = stream.readString();

                Log::printf(LOG_INFO, "%s authenticating", username.c_str());
                remotePeer->type = Peer::ConnectedPlayer;
                remotePeer->playerEntity = (Player*)instantiate(playerType);
                remotePeer->playerEntity->remotePeerId.set(remotePeer->peerId);
                remotePeer->playerEntity->displayName.set(username);
                for (auto& e : entities)
                  remotePeer->pendingNewIds.push_back(e.first);
              } else {
                throw std::runtime_error(
                    "Received AuthenticatePacket on frontend");
              }
              break;
            case NewIdPacket:
              if (backend) {
                throw std::runtime_error("Received NewIdPacket on backend");
              } else {
                int numEntities = stream.read<int>();
                for (int i = 0; i < numEntities; i++) {
                  EntityId id = stream.read<EntityId>();
                  std::string typeName = stream.readString();
                  Entity* ent = instantiate(typeName, id);
                  ent->deserialize(stream);

                  Log::printf(LOG_DEBUG, "created %s", typeName.c_str());
                }
              }
              break;
            case DelIdPacket:
              if (backend) {
                throw std::runtime_error("Received DelIdPacket on backend");
              } else {
                int numEntities = stream.read<int>();
                for (int i = 0; i < numEntities; i++) {
                  EntityId id = stream.read<EntityId>();
                  deleteEntity(id);
                }
              }
              break;
            case NewPeerPacket:
              if (backend) {
                throw std::runtime_error("Received NewPeerPacket on backend");
              } else {
                int peerId = stream.read<int>();
                Peer np;
                np.peer = NULL;
                np.playerEntity = NULL;
                np.peerId = peerId;
                peers[np.peerId] = np;
              }
              break;
            case DelPeerPacket:
              if (backend) {
                throw std::runtime_error("Received DelPeerPacket on backend");
              } else {
                int peerId = stream.read<int>();
                auto it = peers.find(peerId);
                if (it != peers.end()) {
                  peers.erase(it);
                } else {
                  throw std::runtime_error(
                      "Attempted to delete non-existent peer");
                }
              }
            case DisconnectPacket:
              if (backend) {
                throw std::runtime_error(
                    "Received DisconnectPacket on backend");
              } else {
                std::string message = stream.readString();
                Log::printf(LOG_INFO, "Disconnected from server (%s)",
                            message.c_str());
                enet_peer_disconnect_now(localPeer.peer, 0);
                localPeer.type = Peer::Unconnected;
              }
              break;
            default:
              Log::printf(LOG_WARN, "%s: Unknown packet %i",
                          backend ? "Backend" : "Frontend", packetId);
              break;
          }
        } catch (std::exception& e) {
          Log::printf(LOG_ERROR, "%s: Error in ENET_EVENT_TYPE_RECEIVE: %s",
                      backend ? "Backend" : " Frontend", e.what());
        }
      } break;
      case ENET_EVENT_TYPE_DISCONNECT: {
        if (backend) {
          Peer* peer = (Peer*)event.peer->data;
          if (peer) {
            Log::printf(LOG_INFO, "Peer disconnecting (reason: %s)",
                        disconnectReasons[event.data]);

            BitStream peerRemoving;
            peerRemoving.write<PacketId>(DelPeerPacket);
            peerRemoving.write<int>(peer->peerId);
            deleteEntity(peer->playerEntity->getEntityId());

            peers.erase(peer->peerId);
            enet_host_broadcast(
                host, 0, peerRemoving.createPacket(ENET_PACKET_FLAG_RELIABLE));
          } else {
            // IDK
          }
        } else {
        }
      } break;
      case ENET_EVENT_TYPE_CONNECT: {
        char hostname[64];
        enet_address_get_host(&event.peer->address, hostname, 64);
        Log::printf(LOG_INFO, "%s: Connection from %s:%i",
                    backend ? "Backend" : "Frontend", hostname,
                    event.peer->address.port);

        if (backend) {
          Peer np;
          np.type = Peer::Undifferentiated;
          np.peer = event.peer;
          np.peerId = lastPeerId++;
          np.playerEntity = NULL;

          BitStream newPeerPacketStream;
          newPeerPacketStream.write<PacketId>(NewPeerPacket);
          newPeerPacketStream.write<int>(np.peerId);
          ENetPacket* packet =
              newPeerPacketStream.createPacket(ENET_PACKET_FLAG_RELIABLE);
          for (auto& peer : peers) {
            enet_peer_send(peer.second.peer, 0, packet);
          }

          peers[np.peerId] = np;
          event.peer->data = &peers[np.peerId];

          BitStream welcomePacketStream;
          welcomePacketStream.write<PacketId>(WelcomePacket);
          welcomePacketStream.write<int>(np.peerId);
          welcomePacketStream.write<size_t>(ticks);
          enet_peer_send(
              event.peer, NETWORK_STREAM_META,
              welcomePacketStream.createPacket(ENET_PACKET_FLAG_RELIABLE));
        } else {
          localPeer.peer = event.peer;
          localPeer.type = Peer::ConnectedPlayer;
        }
      } break;
      default:
        break;
    }
  }

  if (backend) {
    for (auto& peer : peers) {
      if (int pendingNewIds = peer.second.pendingNewIds.size()) {
        BitStream newIdStream;
        newIdStream.write<PacketId>(NewIdPacket);
        newIdStream.write<int>(pendingNewIds);
        for (auto id : peer.second.pendingNewIds) {
          newIdStream.write<EntityId>(id);
          Entity* ent = entities[id].get();
          newIdStream.writeString(ent->getTypeName());
          ent->serialize(newIdStream);
        }
        enet_peer_send(peer.second.peer, NETWORK_STREAM_ENTITY,
                       newIdStream.createPacket(ENET_PACKET_FLAG_RELIABLE));
        peer.second.pendingNewIds.clear();
      }

      if (int pendingDelIds = peer.second.pendingDelIds.size()) {
        BitStream delIdStream;
        delIdStream.write<PacketId>(DelIdPacket);
        delIdStream.write<int>(pendingDelIds);
        for (auto id : peer.second.pendingDelIds) {
          delIdStream.write<EntityId>(id);
        }
        enet_peer_send(peer.second.peer, NETWORK_STREAM_ENTITY,
                       delIdStream.createPacket(ENET_PACKET_FLAG_RELIABLE));
        peer.second.pendingDelIds.clear();
      }
    }
  } else {
    if (localPeer.playerEntity) {
      if (localPeer.playerEntity->remotePeerId.get() != localPeer.peerId) {
        localPeer.playerEntity = nullptr;
      }
    }
    if (!localPeer.playerEntity) {
      for (auto& pair : entities) {
        Entity* entity = pair.second.get();
        if (Player* playerEntity = dynamic_cast<Player*>(entity)) {
          if (playerEntity->remotePeerId.get() == localPeer.peerId) {
            Log::printf(LOG_DEBUG, "Found player entity id %i, username %s",
                        playerEntity->remotePeerId.get(),
                        playerEntity->displayName.get().c_str());
            localPeer.playerEntity = playerEntity;
          }
        }
      }
    }
  }

  ticks++;
}

void NetworkManager::start(int port) {
  if (host) throw std::runtime_error("Already hosting");
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;
  host = enet_host_create(&address, 32, 2, 0, 0);

  backend = true;

  if (host == nullptr)
    throw std::runtime_error("enet_host_create returned NULL");
}

void NetworkManager::connect(std::string address, int port) {
  if (host) throw std::runtime_error("Already hosting");
  ENetAddress _address;
  enet_address_set_host(&_address, address.c_str());
  _address.port = port;

  backend = false;

  host = enet_host_create(NULL, 1, 2, 0, 0);
  if (host == nullptr)
    throw std::runtime_error("enet_host_create returned NULL");

  localPeer.peer = enet_host_connect(host, &_address, 2, 0);
  if (localPeer.peer == nullptr)
    throw std::runtime_error("enet_host_connect returned NULL");

  Log::printf(LOG_INFO, "Started server on port %i", port);
}

void NetworkManager::registerConstructor(EntityConstructorFunction func,
                                         std::string type) {
  Log::printf(LOG_DEBUG, "Registered entity type %s", type.c_str());
  constructors[type] = func;
}

void NetworkManager::deleteEntity(EntityId id) {
  auto it = entities.find(id);
  if (it != entities.end()) {
    if (backend) {
      for (auto& peer : peers) {
        if (peer.second.playerEntity) peer.second.pendingDelIds.push_back(id);
      }
    }
    entities.erase(it);
  } else {
    Log::printf(LOG_ERROR, "Attempt to delete entity id %i", id);
    throw std::runtime_error("Invalid delete entity id");
  }
}

Entity* NetworkManager::instantiate(std::string typeName, int _id) {
  auto it = constructors.find(typeName);
  if (it != constructors.end()) {
    EntityId id = 0;
    if (_id == -1) {
      id = lastId++;
    } else {
      id = _id;
    }
    entities[id].reset(constructors[typeName](this, id));
    if (backend) {
      for (auto& peer : peers) {
        if (peer.second.playerEntity) peer.second.pendingNewIds.push_back(id);
      }
    }
    return entities[id].get();
  } else {
    Log::printf(LOG_ERROR, "Could not instantiate entity of type %s",
                typeName.c_str());
    throw std::runtime_error("Could not instantiate unknown entity");
  }
}

Entity* NetworkManager::findEntityByType(std::string typeName) {
  for (auto& entity : entities) {
    if (entity.second->getTypeName() == typeName) return entity.second.get();
  }
  return NULL;
}
}  // namespace rdm::network
