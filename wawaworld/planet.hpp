#pragma once
#include <memory>

#include "gfx/base_types.hpp"
#include "gfx/engine.hpp"
namespace ww {
class Planet {
  struct Region {};

  std::unique_ptr<rdm::gfx::BaseTexture> planetTextureColor;
  int genSeed;

 public:
  Planet();

  void generate(int seed);
  void initGfx(rdm::gfx::Engine* engine);

  rdm::gfx::BaseTexture* getColor() { return planetTextureColor.get(); }
};
}  // namespace ww
