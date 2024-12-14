#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "graph.hpp"
#include "network/network.hpp"
#include "physics.hpp"
#include "scheduler.hpp"
#include "script/context.hpp"

namespace rdm {
struct WorldConstructorSettings {
  bool network;
  bool physics;

  WorldConstructorSettings() {
    network = false;
    physics = false;
  }
};

class World {
  friend class WorldJob;
  friend class WorldTitleJob;

  std::unique_ptr<Graph> graph;
  std::unique_ptr<PhysicsWorld> physics;
  std::unique_ptr<network::NetworkManager> networkManager;
  std::unique_ptr<Scheduler> scheduler;
  std::unique_ptr<script::Context> scriptContext;
  std::string title;
  bool running;
  double time;

  void tick();

 public:
  World(WorldConstructorSettings settings = WorldConstructorSettings());
  ~World();

  Signal<> stepping;
  Signal<> stepped;
  Signal<std::string> changingTitle;

  std::mutex worldLock;  // lock when writing to world state

  void setTitle(std::string title);

  script::Context* getScriptContext() { return scriptContext.get(); }
  Scheduler* getScheduler() { return scheduler.get(); }
  PhysicsWorld* getPhysicsWorld() { return physics.get(); }
  network::NetworkManager* getNetworkManager() { return networkManager.get(); }
  double getTime() { return time; };

  void setRunning(bool running) { this->running = running; }
  bool getRunning() { return running; };
};
}  // namespace rdm
