#include "renderpass.hpp"
namespace rdm::gfx {
void RenderPass::add(RenderList list) { lists.push_back(list); }
void RenderPass::render(gfx::Engine* engine) {
  for (auto list : lists) {
    list.render(engine);
  }

  lists.clear();  // need resubmitting
}
}  // namespace rdm::gfx
