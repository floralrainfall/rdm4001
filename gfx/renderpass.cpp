#include "renderpass.hpp"

#include <format>

#include "engine.hpp"
namespace rdm::gfx {
void RenderPass::add(RenderList list) { lists.push_back(list); }
void RenderPass::render(gfx::Engine* engine) {
  for (int i = 0; i < lists.size(); i++) {
    engine->getDevice()->dbgPushGroup(std::format("Command {}", i));
    lists[i].render(engine);
    engine->getDevice()->dbgPopGroup();
  }

  lists.clear();  // need resubmitting
}
}  // namespace rdm::gfx
