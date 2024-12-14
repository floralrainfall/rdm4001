#include "context.hpp"

#include "api.hpp"
#include "script.hpp"
#include "script/my_basic.h"
namespace rdm::script {
Context::Context(World* world) { this->world = world; }

void Context::setGfxEngine(gfx::Engine* engine) { this->engine = engine; }

void Context::setContext(Script* script) {
  struct mb_interpreter_t* s = script->getInterpreter();
  mb_set_userdata(s, this);
  API::registerApi(s);
}
}  // namespace rdm::script
