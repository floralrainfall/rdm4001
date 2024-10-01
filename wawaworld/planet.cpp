#include "planet.hpp"

#include <cmath>

#include "gfx/base_device.hpp"
#include "gfx/engine.hpp"
namespace ww {
Planet::Planet() {}

void Planet::generate(int seed) {
#define REGION_STEP 0.1

  for (double x = 0.0; x < (M_PI * 2.0); x += REGION_STEP) {
    for (double y = 0.0; y < (M_PI * 2.0); y += REGION_STEP) {
      for (double z = 0.0; z < (M_PI * 2.0); z += REGION_STEP) {
      }
    }
  }
}

void Planet::initGfx(rdm::gfx::Engine* engine) {}
}  // namespace ww
