#pragma once

#include "script/my_basic.h"
#include "signal.hpp"
namespace rdm {
class World;

namespace gfx {
class Engine;
};
};  // namespace rdm

namespace rdm::script {
class Script;
class Context {
  World* world;
  gfx::Engine* engine;

 public:
  Context(World* world);

  Signal<struct mb_interpreter_t*> setContextCall;

  World* getWorld() { return world; };
  gfx::Engine* getEngine() { return engine; };

  void setGfxEngine(gfx::Engine* engine);
  void setContext(Script* script);
};
};  // namespace rdm::script
