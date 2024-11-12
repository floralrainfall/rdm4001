#pragma once
#include "game.hpp"
#include "network/network.hpp"

namespace ww {
using namespace rdm;
struct WGamePrivate;
class WGame : public Game {
  WGamePrivate* game;

 public:
  WGame();
  ~WGame();

  void addEntityConstructors(network::NetworkManager* manager);

  virtual void initialize();
  virtual void initializeClient();
  virtual void initializeServer();

  virtual size_t getGameVersion();
};
};  // namespace ww
