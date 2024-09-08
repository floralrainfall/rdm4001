#pragma once
#include "base_context.hpp"
#include "base_device.hpp"
#include "entity.hpp"
#include "signal.hpp"
#include "gfx/gui/gui.hpp"
#include <memory>
#include <easy/profiler.h>

namespace rdm {
class World;
}

namespace rdm::gfx {
class Engine {
  friend class RenderJob;

  std::unique_ptr<BaseContext> context;
  std::unique_ptr<BaseDevice> device;
  std::unique_ptr<gui::GuiManager> gui;
  std::vector<std::unique_ptr<Entity>> entities;

  bool isInitialized;
  void initialize();
  void render();
  void stepped();

  std::unique_ptr<MaterialCache> materialCache;
  World* world;
public:
  Engine(World* world, void* hwnd);

  Signal<> renderStepped;

  Entity* addEntity(std::unique_ptr<Entity> entity);

  BaseContext* getContext() { return context.get(); }
  BaseDevice* getDevice() { return device.get(); }
  MaterialCache* getMaterialCache() { return materialCache.get(); }
  gui::GuiManager* getGuiManager() { return gui.get(); }
};
}