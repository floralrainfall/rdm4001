#include "world.hpp"

#include <chrono>
#include <format>

#include "input.hpp"
#include "logging.hpp"
#include "physics.hpp"
#include "scheduler.hpp"

namespace rdm {
class WorldJob : public SchedulerJob {
  World* world;

 public:
  WorldJob(World* world) : SchedulerJob("World", false), world(world) {}

  virtual Result step() {
    using namespace std::chrono_literals;
    if (!world->getRunning()) return Cancel;

    if (getStats().schedulerId == 0) Input::singleton()->flushEvents();
    world->tick();

    return Stepped;
  }

  virtual void error(std::exception& e) { world->running = false; }
};

class WorldTitleJob : public SchedulerJob {
  World* world;

 public:
  WorldTitleJob(World* world) : SchedulerJob("WorldTitleJob"), world(world) {}

  virtual double getFrameRate() { return 1.0; }

  virtual Result step() {
    std::string title = world->title;
    std::string fpsStatus = "";
    if (SchedulerJob* worldJob = world->scheduler->getJob("World")) {
      fpsStatus =
          std::format("W: {:0.2f}", 1.0 / worldJob->getStats().totalDeltaTime);
    }
    if (SchedulerJob* physicsJob = world->scheduler->getJob("Physics")) {
      fpsStatus += std::format(" P: {:0.2f}",
                               1.0 / physicsJob->getStats().totalDeltaTime);
    }
    if (SchedulerJob* renderJob = world->scheduler->getJob("Render")) {
      fpsStatus += std::format(" R: {:0.2f}",
                               1.0 / renderJob->getStats().totalDeltaTime);
    }
    world->changingTitle.fire(std::format("{} ({})", title, fpsStatus));

    return Stepped;
  }
};

World::World() {
  title = "A rdm presentation";

  scheduler.reset(new Scheduler());
  scheduler->addJob(new WorldJob(this));
  scheduler->addJob(new WorldTitleJob(this));

  physics.reset(new PhysicsWorld(this));
  networkManager.reset(new network::NetworkManager(this));

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

void World::setTitle(std::string title) { this->title = title; }
}  // namespace rdm
