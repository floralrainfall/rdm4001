#pragma once
#include <SDL2/SDL.h>

#include <memory>

#include "gfx/engine.hpp"
#include "sound.hpp"
#include "state.hpp"
#include "world.hpp"

#define ENGINE_VERSION 0x1000

namespace rdm {
class Game {
 protected:
  std::unique_ptr<World> worldServer;
  std::unique_ptr<World> world;
  std::unique_ptr<gfx::Engine> gfxEngine;
  std::unique_ptr<SoundManager> soundManager;
  std::unique_ptr<GameState> gameState;

 private:
  SDL_Window* window;
  bool ignoreNextMouseMoveEvent;
  WorldConstructorSettings worldSettings;

 public:
  Game();
  virtual ~Game();

  // start the game state mode
  void startGameState(GameStateConstructorFunction f);

  WorldConstructorSettings& getWorldConstructorSettings() {
    return worldSettings;
  }

  // call before accessing world
  void startClient();
  // call before accessing worldServer
  void startServer();

  void lateInitServer();
  void stopServer();

  virtual void initialize() = 0;
  virtual void initializeClient() {};
  virtual void initializeServer() {};

  const char* copyright();

  size_t getVersion();
  virtual size_t getGameVersion() { return 0; };

  void earlyInit();
  void pollEvents();

  void mainLoop();

  World* getWorld() { return world.get(); }
  World* getServerWorld() { return worldServer.get(); }

  SoundManager* getSoundManager() { return soundManager.get(); }
  gfx::Engine* getGfxEngine() { return gfxEngine.get(); }
};
}  // namespace rdm
