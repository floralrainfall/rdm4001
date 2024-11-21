#pragma once
#include "game.hpp"
#include "network/network.hpp"
#include "sound.hpp"
namespace rt {
class RoadTrip : public rdm::Game {
  rdm::SoundEmitter* mainMenuEmitter;

  void addEntityConstructors(rdm::network::NetworkManager* manager);

 public:
  virtual void initialize();
  virtual void initializeClient();
  virtual void initializeServer();
};
}  // namespace rt
