#include "roadtrip.hpp"
#include "settings.hpp"
int main(int argc, char** argv) {
  rdm::Settings::singleton()->parseCommandLine(argv, argc);
  rt::RoadTrip game;
  game.mainLoop();
  rdm::Settings::singleton()->save();
}
