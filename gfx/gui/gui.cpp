#include "gfx/gui/gui.hpp"
#include "gfx/engine.hpp"

namespace rdm::gfx::gui {
GuiManager::GuiManager(gfx::Engine* engine) {
  gfx::MaterialCache* cache = engine->getMaterialCache();
  panel = cache->getOrLoad("GuiPanel").value();
  text = cache->getOrLoad("GuiText").value();
  image = cache->getOrLoad("GuiImage").value();

  engine->renderStepped.listen([this]{render();});
}

void GuiManager::render() {
  
}
}