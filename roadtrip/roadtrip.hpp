#pragma once
#include "game.hpp"
#include "network/network.hpp"
namespace rt {
class RoadTrip : public rdm::Game {
  void addEntityConstructors(rdm::network::NetworkManager* manager);

 public:
  virtual void initialize();
  virtual void initializeClient();
  virtual void initializeServer();
};
}  // namespace rt
