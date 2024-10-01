#pragma once
#include <SDL2/SDL.h>

#include <memory>

#include "gfx/engine.hpp"
#include "world.hpp"

namespace rdm {
class Game {
 protected:
  std::unique_ptr<World> world;
  std::unique_ptr<gfx::Engine> gfxEngine;

 private:
  SDL_Window* window;

 public:
  Game();

  virtual void initialize() = 0;

  void mainLoop();

  World* getWorld() { return world.get(); }
};
}  // namespace rdm