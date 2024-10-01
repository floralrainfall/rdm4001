#pragma once
#include <string>
#include <enet/enet.h>
#include "player.hpp"

namespace rdm {
class World;
}

namespace rdm::network {
class NetworkManager {
  friend class NetworkJob;

  ENetHost* host;
  Player localPlayer;
  World* world;
  bool backend;
public:
  NetworkManager(World* world);
  ~NetworkManager();

  void service();

  void start(int port = 7938);

  void connect(std::string address, int port = 7938);
  void requestDisconnect();
};
}