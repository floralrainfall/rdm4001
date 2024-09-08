#pragma once
#include <memory>
#include "world.hpp"
#include "gfx/engine.hpp"
#include <SDL2/SDL.h>

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
}