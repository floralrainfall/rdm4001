#include "engine.hpp"
#include "gl_device.hpp"
#include "scheduler.hpp"
#include "world.hpp"
#include <easy/profiler.h>

namespace rdm::gfx {
class RenderJob : public SchedulerJob {
  Engine* engine;

public:
  RenderJob(Engine* engine) : SchedulerJob("Render") 
                            , engine(engine) {}

  virtual Result step() {
    BaseDevice* device = engine->device.get();

    {
      EASY_BLOCK("Lock Context");
      std::scoped_lock lock(engine->context->getMutex());
      EASY_END_BLOCK;

      engine->context->setCurrent();

      EASY_BLOCK("Viewport");
      glm::ivec2 fbSize = engine->context->getBufferSize();
      device->viewport(0, 0, fbSize.x, fbSize.y);
      device->clear(0.3, 0.3, 0.3, 0.0);
      EASY_END_BLOCK;

      if(!engine->isInitialized)
        engine->initialize();

      EASY_BLOCK("Render");
      engine->render();
      EASY_END_BLOCK;

      EASY_BLOCK("Swap Buffers");
      engine->context->swapBuffers();
      EASY_END_BLOCK;

      engine->context->unsetCurrent();
    }

    return Stepped;
  }
};

Engine::Engine(World* world, void* hwnd) {
  context = std::unique_ptr<BaseContext>(new GLContext(hwnd));
  std::scoped_lock lock(context->getMutex());
  device = std::unique_ptr<BaseDevice>(new GLDevice(dynamic_cast<GLContext*>(context.get())));
  materialCache = std::unique_ptr<MaterialCache>(new MaterialCache(device.get()));

  world->getScheduler()->addJob(new RenderJob(this));
  world->stepped.listen([this]{stepped();});
  isInitialized = false;
}

void Engine::stepped() {

}

void Engine::render() {
  for(int i = 0; i < entities.size(); i++) {
    Entity* ent = entities[i].get();
    ent->render(device.get());
  }

  renderStepped.fire();
}

void Engine::initialize() {
  gui = std::unique_ptr<gui::GuiManager>(new gui::GuiManager(this));
  isInitialized = true;
}

Entity* Engine::addEntity(std::unique_ptr<Entity> entity) {
  entities.push_back(std::move(entity));
  return (entities.back()).get();
}
}