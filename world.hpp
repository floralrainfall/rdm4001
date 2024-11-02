#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "graph.hpp"
#include "network/network.hpp"
#include "physics.hpp"
#include "scheduler.hpp"

namespace rdm {
class World {
  friend class WorldJob;
  friend class WorldTitleJob;

  std::unique_ptr<Graph> graph;
  std::unique_ptr<PhysicsWorld> physics;
  std::unique_ptr<network::NetworkManager> networkManager;
  std::unique_ptr<Scheduler> scheduler;
  std::string title;
  bool running;

  void tick();

 public:
  World();

  Signal<> stepping;
  Signal<> stepped;
  Signal<std::string> changingTitle;

  std::mutex worldLock;  // lock when writing to world state

  void setTitle(std::string title);

  Scheduler* getScheduler() { return scheduler.get(); }
  PhysicsWorld* getPhysicsWorld() { return physics.get(); }
  network::NetworkManager* getNetworkManager() { return networkManager.get(); }
  bool getRunning() { return running; };
};
}  // namespace rdm
