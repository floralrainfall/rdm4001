#pragma once

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

  World* getWorld() { return world; };
  gfx::Engine* getEngine() { return engine; };

  void setGfxEngine(gfx::Engine* engine);
  void setContext(Script* script);
};
};  // namespace rdm::script
