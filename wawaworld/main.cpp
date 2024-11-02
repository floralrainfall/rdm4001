#ifndef DISABLE_EASY_PROFILER
#include <easy/profiler.h>
#endif

#include "logging.hpp"
#include "settings.hpp"
#include "wgame.hpp"

int main(int argc, char** argv) {
#ifndef DISABLE_EASY_PROFILER
  EASY_MAIN_THREAD;
  EASY_PROFILER_ENABLE;
  profiler::startListen();
#endif
  rdm::Settings::singleton()->parseCommandLine(argv, argc);

  ww::WGame game;
  game.mainLoop();

  rdm::Log::printf(rdm::LOG_INFO, "Goodbye");
  rdm::Settings::singleton()->save();
#ifndef DISABLE_EASY_PROFILER
  rdm::Log::printf(rdm::LOG_INFO, "Saved profiling information");
  if (rdm::Settings::singleton()->getHintDs())
    profiler::dumpBlocksToFile("test_profile_ds.prof");
  else
    profiler::dumpBlocksToFile("test_profile.prof");
#endif
}
