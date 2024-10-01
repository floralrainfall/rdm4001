#pragma once

#include <memory>
#include <mutex>

#include "graph.hpp"
#include "scheduler.hpp"

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

  std::mutex worldLock;  // lock when writing to world state

  Scheduler* getScheduler() { return scheduler.get(); }
  bool getRunning() { return running; };
};
}  // namespace rdm