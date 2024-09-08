#pragma once
#include <memory>
#include "gfx/material.hpp"
namespace rdm::gfx {
class Engine;
};

namespace rdm::gfx::gui {
class GuiManager {
  gfx::Engine* engine;
  std::shared_ptr<gfx::Material> panel;
  std::shared_ptr<gfx::Material> image;
  std::shared_ptr<gfx::Material> text;

  void render();
public:
  GuiManager(gfx::Engine* engine);
};
};