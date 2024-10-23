#include <stdlib.h>
#include <dlfcn.h>
#include <cstdlib>
#include "game.hpp"
#include "settings.hpp"
#include "logging.hpp"
#include "launcher.hpp"

int main(int argc, char** argv) {
  rdm::Settings::singleton()->parseCommandLine(argv, argc);
  std::string gamePath = rdm::Settings::singleton()->getGamePath();
  if(gamePath == "") {
    rdm::Log::printf(rdm::LOG_FATAL, "Please specify game path with -g or --game");
    exit(EXIT_FAILURE);
  }
  rdm::Game* game;

  void* mod = dlopen(gamePath.c_str(), RTLD_NOW | RTLD_GLOBAL | RTLD_DEEPBIND);
  if(!mod) {
    rdm::Log::printf(rdm::LOG_FATAL, "Could not open module %s (dlerror: %s)", gamePath.c_str(), dlerror());
    exit(EXIT_FAILURE);
  }

  launcher::Exports* exports = (launcher::Exports*)dlsym(mod, "__RDM4001_LAUNCHER_EXPORTS");
  if(!exports) {
    rdm::Log::printf(rdm::LOG_FATAL, "Could not open launcher exports (dlerror: %s)", dlerror());
    exit(EXIT_FAILURE);
  }
  
  game = exports->init(LAUNCHER_VERSION);
  if(!game) {
    rdm::Log::printf(rdm::LOG_FATAL, "Error creating game = 0");
    exit(EXIT_FAILURE);
  }

  game->mainLoop();
  rdm::Settings::singleton()->save();
}
