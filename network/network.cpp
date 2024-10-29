#include "network.hpp"

#include <stdexcept>

#include "logging.hpp"
#include "network/entity.hpp"
#include "scheduler.hpp"
#include "world.hpp"

namespace rdm::network {
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
  host = 0;
  this->world = world;
  world->getScheduler()->addJob(new NetworkJob(this));

  registerConstructor(EntityConstructor<Player>, "Player");

  BitStream stream;
  ReplicateProperty<std::string> p;
  p.set("Hi world");
  p.serialize(stream);
  BitStream stream2(stream);
  ReplicateProperty<std::string> p2;
  p2.deserialize(stream2);
  Log::printf(LOG_DEBUG, "%s", p2.get().c_str());
}

NetworkManager::~NetworkManager() {
  if (host) enet_host_destroy(host);
  enet_deinitialize();
}

void NetworkManager::service() {
  if (!host) return;

  ENetEvent event;
  while (enet_host_service(host, &event, 1) > 0) {
    switch (event.type) {
      case ENET_EVENT_TYPE_RECEIVE:

        break;
      case ENET_EVENT_TYPE_DISCONNECT: {
        if (backend) {
        } else {
        }
      } break;
      case ENET_EVENT_TYPE_CONNECT: {
        char hostname[64];
        enet_address_get_host(&event.peer->address, hostname, 64);
        Log::printf(LOG_INFO, "Connection from %s:%i", hostname,
                    event.peer->address.port);

        if (backend) {
          Peer* np = new Peer();
          np->type = Peer::Undifferentiated;
          event.peer->data = np;
        } else {
          localPeer.type = Peer::ConnectedPlayer;
        }
      } break;
      default:
        break;
    }
  }

  if (backend) {
    for (auto peer : peers) {
    }
  } else {
    if (localPeer.playerEntity) {
      if (localPeer.playerEntity->getRemotePeerId() != localPeer.peerId) {
        localPeer.playerEntity = nullptr;
      }
    }
    if (!localPeer.playerEntity) {
      for (auto& pair : entities) {
        Entity* entity = pair.second.get();
        if (Player* playerEntity = dynamic_cast<Player*>(entity)) {
          if (playerEntity->getRemotePeerId() == localPeer.peerId) {
            localPeer.playerEntity = playerEntity;
          }
        }
      }
    }
  }
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
}

void NetworkManager::registerConstructor(EntityConstructorFunction func,
                                         std::string type) {
  Log::printf(LOG_DEBUG, "Registered entity type %s", type.c_str());
  constructors[type] = func;
}

Entity* NetworkManager::instantiate(std::string typeName) {
  auto it = constructors.find(typeName);
  if (it != constructors.end()) {
    EntityId id = lastId++;
    entities[id].reset(constructors[typeName](this, id));
    return entities[id].get();
  } else {
    Log::printf(LOG_ERROR, "Could not instantiate entity of type %s",
                typeName.c_str());
    throw std::runtime_error("Could not instantiate unknown entity");
  }
}
}  // namespace rdm::network
