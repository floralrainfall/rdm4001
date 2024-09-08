#pragma once

#include <memory>
#include "scheduler.hpp"
#include "graph.hpp"
#include <mutex>

namespace rdm {
class World {
  friend class WorldJob;

  std::unique_ptr<Scheduler> scheduler;
  std::unique_ptr<Graph> graph;
  bool running;

  void tick();
public:
  World();

  Signal<> stepping;
  Signal<> stepped;

  std::mutex worldLock; // lock when writing to world state

  Scheduler* getScheduler() { return scheduler.get(); }
  bool getRunning() { return running; };
};
}