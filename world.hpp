#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "graph.hpp"
#include "scheduler.hpp"
#include "physics.hpp"

namespace rdm {
class World {
  friend class WorldJob;
  friend class WorldTitleJob;

  std::unique_ptr<Scheduler> scheduler;
  std::unique_ptr<Graph> graph;
  std::unique_ptr<PhysicsWorld> physics;
  bool running;

  void tick();

 public:
  World();

  Signal<> stepping;
  Signal<> stepped;
  Signal<std::string> changingTitle;

  std::mutex worldLock;  // lock when writing to world state

  Scheduler* getScheduler() { return scheduler.get(); }
  PhysicsWorld* getPhysicsWorld() { return physics.get(); }
  bool getRunning() { return running; };
};
}  // namespace rdm
