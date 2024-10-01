#include "network.hpp"
#include <stdexcept>
#include "scheduler.hpp"
#include "logging.hpp"
#include "world.hpp"

namespace rdm::network {
class NetworkJob : public SchedulerJob {
  NetworkManager* netmanager;

public:
  NetworkJob(NetworkManager* netmanager) : SchedulerJob("Network")
                                         , netmanager(netmanager) {}

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
}

NetworkManager::~NetworkManager() {
  if(host)
    enet_host_destroy(host);
  enet_deinitialize();
}

void NetworkManager::service() {
  if(host)
    return;

  ENetEvent event;
  while(enet_host_service(host, &event, 1) > 0) {
    switch(event.type) {
    case ENET_EVENT_TYPE_RECEIVE:

      break;
    case ENET_EVENT_TYPE_DISCONNECT:
      {
        if(backend) {

        } else {

        }
      }
      break;
    case ENET_EVENT_TYPE_CONNECT:
      {
        char hostname[64];
        enet_address_get_host(&event.peer->address, hostname, 64);
        Log::printf(LOG_INFO, "Connection from %s:%i", hostname, event.peer->address.port);

        if(backend) {
          Player* np = new Player();
          np->type = Player::Undifferentiated;
          event.peer->data = np;
        } else {
          localPlayer.type = Player::ConnectedPlayer;

        }
      }
      break;
    default:
      break;
    }
  }
}

void NetworkManager::start(int port) {
  if(host)
    throw std::runtime_error("Already hosting");
  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = port;
  host = enet_host_create(&address, 32, 2, 0, 0);

  backend = true;

  if(host == nullptr)
    throw std::runtime_error("enet_host_create returned NULL");
}

void NetworkManager::connect(std::string address, int port) {
  if(host)
    throw std::runtime_error("Already hosting");
  ENetAddress _address;
  enet_address_set_host(&_address, address.c_str());
  _address.port = port;

  backend = false;

  host = enet_host_create(NULL, 1, 2, 0, 0);
  if(host == nullptr)
    throw std::runtime_error("enet_host_create returned NULL");
  
  localPlayer.peer = enet_host_connect(host, &_address, 2, 0);
  if(localPlayer.peer == nullptr)
    throw std::runtime_error("enet_host_connect returned NULL");
}
}