#pragma once
#include <SDL2/SDL.h>

#include <memory>
#include <string>

#include "gfx/base_types.hpp"
#include "gfx/material.hpp"
#include "layout.hpp"

namespace rdm::gfx {
class Engine;
};

namespace rdm::gfx::gui {
class GuiManager {
  gfx::Engine* engine;
  std::shared_ptr<gfx::Material> panel;
  std::shared_ptr<gfx::Material> image;
  std::shared_ptr<gfx::Material> text;

  std::unique_ptr<BaseBuffer> squareArrayBuffer;
  std::unique_ptr<BaseBuffer> squareElementBuffer;
  std::unique_ptr<BaseArrayPointers> squareArrayPointers;

  std::vector<std::unique_ptr<BaseTexture>> textures;
  std::map<std::string, Layout> layouts;
  std::map<std::string, void*> fonts;  // sorry it has to be void

  void render();

 public:
  GuiManager(gfx::Engine* engine);

  Layout& getLayout(std::string layout) { return layouts[layout]; }
};
};  // namespace rdm::gfx::gui
