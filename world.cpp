#include "world.hpp"

#include <chrono>
#include <format>

#include "input.hpp"
#include "logging.hpp"
#include "scheduler.hpp"

namespace rdm {
class WorldJob : public SchedulerJob {
  World* world;

 public:
  WorldJob(World* world) : SchedulerJob("World", false), world(world) {}

  virtual Result step() {
    using namespace std::chrono_literals;
    if (!world->getRunning()) return Cancel;
    Input::singleton()->flushEvents();
    world->tick();

    std::string title = "A rdm presentation";
    if(SchedulerJob* renderJob = world->scheduler->getJob("Render")) {
      world->changingTitle.fire(std::format("{} (W: {:0.2f}, R: {:0.2f})", title, 1.0 /getStats().totalDeltaTime, 1.0 / renderJob->getStats().totalDeltaTime));
    } else {
      world->changingTitle.fire(std::format("{} ({:0.2f})", title, 1.0 / getStats().totalDeltaTime));
    }
    return Stepped;
  }

  virtual void error(std::exception& e) { world->running = false; }
};

World::World() {
  scheduler = std::unique_ptr<Scheduler>(new Scheduler());
  scheduler->addJob(new WorldJob(this));
  running = true;

  Input::singleton()->quitSignal.listen([this](InputObject o) {
    Log::printf(LOG_DEBUG, "Received quit signal, waiting for worldLock");
    std::scoped_lock lock(worldLock);
    running = false;
  });
}

void World::tick() {
  stepping.fire();
  
  stepped.fire();
}
}  // namespace rdm
