#include "network.hpp"

#include <enet/enet.h>

#include <stdexcept>

#include "logging.hpp"
#include "network/entity.hpp"
#include "scheduler.hpp"
#include "settings.hpp"
#include "world.hpp"

#ifndef DISABLE_EASY_PROFILER
#include <easy/profiler.h>
#endif

static const char* disconnectReasons[] = {"Disconnect by engine",
                                          "Disconnect by user", "Timeout"};

namespace rdm::network {
Peer::Peer() {
  playerEntity = NULL;
  peer = NULL;
}

static CVar net_rate("net_rate", "60.0",
                     CVARF_SAVE | CVARF_NOTIFY | CVARF_REPLICATE);
static CVar net_service("net_service", "1", CVARF_SAVE);

class NetworkJob : public SchedulerJob {
  NetworkManager* netmanager;

 public:
  NetworkJob(NetworkManager* netmanager)
      : SchedulerJob("Network"), netmanager(netmanager) {}

  virtual double getFrameRate() { return 1.0 / net_rate.getFloat(); }

  virtual Result step() {
    if (netmanager->backend) {
      netmanager->distributedTime = netmanager->getWorld()->getTime();
    } else {
      netmanager->distributedTime += getStats().totalDeltaTime;
    }
    netmanager->service();
    return Stepped;
  }
};

NetworkManager::NetworkManager(World* world) {
  host = NULL;
  this->world = world;
  world->getScheduler()->addJob(new NetworkJob(this));

  localPeer.type = Peer::Unconnected;
  lastId = 0;
  lastPeerId = 0;
  ticks = 0;
  latency = 0.f;

  registerConstructor(EntityConstructor<Player>, "Player");
  playerType = "Player";

  char* username = getenv("USER");
  this->username = username;

  password = "RDMRDMRDM";
  userPassword = "";
}

void NetworkManager::requestDisconnect() {
  BitStream disconnectMessage;
  disconnectMessage.write<PacketId>(DisconnectPacket);
  disconnectMessage.write<int>(0);
  enet_peer_send(localPeer.peer, 0,
                 disconnectMessage.createPacket(ENET_PACKET_FLAG_RELIABLE));
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
        handleDisconnect();
      }
    }
    enet_host_destroy(host);
  }
  host = NULL;
}

void NetworkManager::service() {
  if (!host) return;

#ifndef DISABLE_EASY_PROFILER
  EASY_FUNCTION();
#endif

  ENetEvent event;

#ifndef DISABLE_EASY_PROFILER
  EASY_BLOCK("Network Service");
#endif
  while (enet_host_service(host, &event, net_service.getInt()) > 0) {
#ifndef DISABLE_EASY_PROFILER
    EASY_BLOCK("Handle Service");
#endif
    switch (event.type) {
      case ENET_EVENT_TYPE_RECEIVE: {
        try {
          Peer* remotePeer = (Peer*)event.peer->data;
          BitStream stream(event.packet->data, event.packet->dataLength);
          PacketId packetId = stream.read<PacketId>();
          try {
            switch (packetId) {
              case WelcomePacket:
                if (backend) {
                  throw std::runtime_error("Received WelcomePacket on backend");
                } else {
                  Log::printf(LOG_DEBUG, "Received WelcomePacket");
                  localPeer.peerId = stream.read<int>();
                  size_t _ticks = stream.read<size_t>();
                  Log::printf(LOG_DEBUG, "Tick diff %i vs %i (%i)", ticks,
                              _ticks, _ticks - ticks);
                  distributedTime = stream.read<float>();
                  ticks = _ticks;

                  BitStream authenticateStream;
                  authenticateStream.write<PacketId>(AuthenticatePacket);
                  authenticateStream.writeString(this->username);
                  authenticateStream.writeString(password);
                  authenticateStream.writeString(userPassword);
                  enet_peer_send(localPeer.peer, NETWORK_STREAM_META,
                                 authenticateStream.createPacket(
                                     ENET_PACKET_FLAG_RELIABLE));
                }
                break;
              case AuthenticatePacket:
                if (backend) {
                  std::string username = stream.readString();
                  std::string password = stream.readString();
                  std::string userPassword = stream.readString();
                  if (!userPassword.empty())
                    if (userPassword != this->userPassword) {
                      Log::printf(LOG_INFO, "%s failed userPassword",
                                  username.c_str());
                      break;  // deny user
                    }

                  if (password != this->password) {
                    Log::printf(LOG_INFO, "%s failed password",
                                username.c_str());
                    break;  // deny user
                  } else {
                    Log::printf(LOG_DEBUG, "%s passed password",
                                username.c_str());
                  }

                  Log::printf(LOG_INFO, "%s authenticating", username.c_str());
                  remotePeer->type = Peer::ConnectedPlayer;
                  remotePeer->playerEntity = (Player*)instantiate(playerType);
                  remotePeer->playerEntity->remotePeerId.set(
                      remotePeer->peerId);
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
              case DeltaIdPacket: {
                int numEntities = stream.read<int>();
                int i;
                try {
                  for (i = 0; i < numEntities; i++) {
                    EntityId id = stream.read<EntityId>();
                    Entity* ent = entities[id].get();
                    if (!ent) {
                      Log::printf(LOG_DEBUG, "%i == NULL", id);
                      throw std::runtime_error("ent == NULL");
                    }

                    if (backend) {
                      bool allowed = false;
                      if (Player* player = dynamic_cast<Player*>(ent)) {
                        if (player->remotePeerId.get() == remotePeer->peerId) {
                          allowed = true;
                        }
                      }

                      if (!allowed) {
                        Log::printf(LOG_ERROR,
                                    "Peer %i attempted to modify %s (%i)",
                                    remotePeer->peerId, ent->getTypeName(),
                                    ent->getEntityId());
                        throw std::runtime_error(
                            "Peer not allowed to modify entity");
                      }
                    }

                    if (event.packet->flags & ENET_PACKET_FLAG_RELIABLE) {
                      ent->deserialize(stream);
                      if (backend) addPendingUpdate(id);
                    } else {
                      ent->deserializeUnreliable(stream);
                      if (backend) addPendingUpdateUnreliable(id);
                    }
                  }
                } catch (std::exception& e) {
                  Log::printf(LOG_ERROR, "Error decoding entity %i: (%s)", i,
                              e.what());
                }
              } break;
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
                break;
              case DisconnectPacket:
                if (backend) {
                  int reason = stream.read<int>();
                  Log::printf(LOG_INFO,
                              "Remote requested disconnect for %i (%s)", reason,
                              disconnectReasons[reason]);
                  enet_peer_disconnect(remotePeer->peer, reason);
                } else {
                  std::string message = stream.readString();
                  Log::printf(LOG_INFO, "Disconnected from server (%s)",
                              message.c_str());
                  enet_peer_disconnect_now(localPeer.peer, 0);
                  localPeer.type = Peer::Unconnected;
                  handleDisconnect();
                  enet_host_destroy(host);
                  host = NULL;
                  return;
                }
                break;
              case SignalPacket:

                break;
              case DistributedTimePacket:
                if (backend)
                  throw std::runtime_error("DistributedTimePacket on backend");
                else {
                  float newTime = stream.read<float>();
                  float diff = newTime - distributedTime;
                  if (fabsf(diff) > 0.05) {
                    Log::printf(LOG_WARN,
                                "Updating distributedTime (diff: %f, new: %f)",
                                diff, newTime);
                    distributedTime = newTime;
                    latency = diff;
                  }
                }
                break;
              default:
                Log::printf(LOG_WARN, "%s: Unknown packet %i",
                            backend ? "Backend" : "Frontend", packetId);
                break;
            }
          } catch (std::exception& e) {
            Log::printf(
                LOG_ERROR,
                "%s: Error in ENET_EVENT_TYPE_RECEIVE: %s (packet id: %i)",
                backend ? "Backend" : "Frontend", e.what(), packetId);
          }
        } catch (std::exception& e) {
          Log::printf(LOG_ERROR, "%s: Error in ENET_EVENT_TYPE_RECEIVE: %s",
                      backend ? "Backend" : "Frontend", e.what());
        }
        enet_packet_destroy(event.packet);
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
          }
        } else {
          handleDisconnect();
          localPeer.type = Peer::Unconnected;
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
          welcomePacketStream.write<float>(distributedTime);
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
#ifndef DISABLE_EASY_PROFILER
  EASY_END_BLOCK;
#endif

#ifndef DISABLE_EASY_PROFILER
  EASY_BLOCK("Network Tick");
#endif
  for (auto& entity : entities) {
    entity.second->tick();
  }
#ifndef DISABLE_EASY_PROFILER
  EASY_END_BLOCK;
#endif

  if (backend) {
#ifndef DISABLE_EASY_PROFILER
    EASY_BLOCK("Backend Peer Management");
#endif
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

    if (int _pendingUpdates = pendingUpdates.size()) {
      BitStream deltaIdStream;
      deltaIdStream.write<PacketId>(DeltaIdPacket);
      deltaIdStream.write<int>(_pendingUpdates);
      for (auto id : pendingUpdates) {
        deltaIdStream.write<EntityId>(id);
        Entity* ent = entities[id].get();
        ent->serialize(deltaIdStream);
      }
      ENetPacket* packet =
          deltaIdStream.createPacket(ENET_PACKET_FLAG_RELIABLE);
      for (auto& peer : peers) {
        if (peer.second.type == Peer::ConnectedPlayer)
          enet_peer_send(peer.second.peer, NETWORK_STREAM_ENTITY, packet);
      }
      pendingUpdates.clear();
    }

    if (int _pendingUpdatesUnreliable = pendingUpdatesUnreliable.size()) {
      BitStream deltaIdStream;
      deltaIdStream.write<PacketId>(DeltaIdPacket);
      deltaIdStream.write<int>(_pendingUpdatesUnreliable);
      for (auto id : pendingUpdatesUnreliable) {
        deltaIdStream.write<EntityId>(id);
        Entity* ent = entities[id].get();
        ent->serializeUnreliable(deltaIdStream);
      }
      ENetPacket* packet = deltaIdStream.createPacket(0);
      for (auto& peer : peers) {
        if (peer.second.type == Peer::ConnectedPlayer)
          enet_peer_send(peer.second.peer, NETWORK_STREAM_ENTITY, packet);
      }
      pendingUpdatesUnreliable.clear();
    }

    BitStream timeStream;
    timeStream.write<PacketId>(DistributedTimePacket);
    timeStream.write<float>(distributedTime);
    enet_host_broadcast(host, NETWORK_STREAM_META,
                        timeStream.createPacket(ENET_PACKET_FLAG_RELIABLE));
  } else {
#ifndef DISABLE_EASY_PROFILER
    EASY_BLOCK("Frontend Peer Management");
#endif
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

    if (int _pendingUpdates = pendingUpdates.size()) {
      BitStream deltaIdStream;
      deltaIdStream.write<PacketId>(DeltaIdPacket);
      deltaIdStream.write<int>(_pendingUpdates);
      for (auto id : pendingUpdates) {
        deltaIdStream.write<EntityId>(id);
        Entity* ent = entities[id].get();
        ent->serialize(deltaIdStream);
      }
      ENetPacket* packet =
          deltaIdStream.createPacket(ENET_PACKET_FLAG_RELIABLE);
      enet_peer_send(localPeer.peer, 0, packet);
      pendingUpdates.clear();
    }

    if (int _pendingUpdatesUnreliable = pendingUpdatesUnreliable.size()) {
      BitStream deltaIdStream;
      deltaIdStream.write<PacketId>(DeltaIdPacket);
      deltaIdStream.write<int>(_pendingUpdatesUnreliable);
      for (auto id : pendingUpdatesUnreliable) {
        deltaIdStream.write<EntityId>(id);
        Entity* ent = entities[id].get();
        ent->serializeUnreliable(deltaIdStream);
      }
      ENetPacket* packet = deltaIdStream.createPacket(0);
      enet_peer_send(localPeer.peer, 0, packet);
      pendingUpdatesUnreliable.clear();
    }
  }

  ticks++;
}

static CVar sv_maxpeers("sv_maxpeers", "32", CVARF_SAVE | CVARF_REPLICATE);
static CVar net_inbandwidth("net_inbandwidth", "0", CVARF_SAVE);
static CVar net_outbandwidth("net_outbandwidth", "0", CVARF_SAVE);

void NetworkManager::start(int port) {
  if (host) {
    Log::printf(LOG_WARN, "host = %p", host);
    enet_host_destroy(host);
  }
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;
  host = enet_host_create(&address, sv_maxpeers.getInt(), NETWORK_STREAM_MAX,
                          net_inbandwidth.getInt(), net_outbandwidth.getInt());

  backend = true;

  if (host == nullptr) {
    throw std::runtime_error("enet_host_create returned NULL");
  }

  Log::printf(LOG_INFO, "Started server on port %i", port);
}

void NetworkManager::connect(std::string address, int port) {
  if (host) {
    Log::printf(LOG_WARN, "host = %p", host);
    enet_host_destroy(host);
  }
  ENetAddress _address;
  enet_address_set_host(&_address, address.c_str());
  _address.port = port;

  backend = false;

  host = enet_host_create(NULL, 1, NETWORK_STREAM_MAX, net_inbandwidth.getInt(),
                          net_outbandwidth.getInt());
  if (host == nullptr)
    throw std::runtime_error("enet_host_create returned NULL");

  localPeer.peer = enet_host_connect(host, &_address, 2, 0);
  if (localPeer.peer == nullptr)
    throw std::runtime_error("enet_host_connect returned NULL");

  localPeer.type = Peer::Undifferentiated;
  Log::printf(LOG_INFO, "Connecting to %s:%i", address.c_str(), port);
}

void NetworkManager::handleDisconnect() {
  entities.clear();
  peers.clear();
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
    Entity* ent = constructors[typeName](this, id);
    if (!ent) {
      Log::printf(LOG_ERROR, "Constructor for %s is NULL", typeName.c_str());
      throw std::runtime_error("Could not instantiate entity");
    }
    entities[id].reset(ent);
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

std::vector<Entity*> NetworkManager::findEntitiesByType(std::string typeName) {
  std::vector<Entity*> ret;
  for (auto& entity : entities) {
    if (entity.second->getTypeName() == typeName)
      ret.push_back(entity.second.get());
  }
  return ret;
}

void NetworkManager::initialize() { enet_initialize(); }

void NetworkManager::deinitialize() { enet_deinitialize(); }
}  // namespace rdm::network
