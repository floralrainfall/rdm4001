#include "logging.hpp"
#include "settings.hpp"
#include "wgame.hpp"

int main(int argc, char** argv) {
  rdm::Settings::singleton()->parseCommandLine(argv, argc);

  ww::WGame game;
  game.mainLoop();

  rdm::Log::printf(rdm::LOG_INFO, "Goodbye");
  rdm::Settings::singleton()->save();
}
