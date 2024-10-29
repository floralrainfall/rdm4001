#pragma once
#include <SDL2/SDL.h>

#include <memory>

#include "gfx/engine.hpp"
#include "world.hpp"

namespace rdm {
class Game {
 protected:
  std::unique_ptr<World> worldServer;
  std::unique_ptr<World> world;
  std::unique_ptr<gfx::Engine> gfxEngine;

 private:
  SDL_Window* window;

 public:
  Game();

  // call before accessing world
  void startClient();
  // call before accessing worldServer
  void startServer();

  virtual void initialize() = 0;
  virtual void initializeClient() {};
  virtual void initializeServer() {};

  void mainLoop();

  World* getWorld() { return world.get(); }
  World* getServerWorld() { return worldServer.get(); }
};
}  // namespace rdm
