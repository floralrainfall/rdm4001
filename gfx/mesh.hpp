#pragma once
#include "base_types.hpp"
#include "base_device.hpp"
#include "engine.hpp"
#include <memory>

namespace rdm::gfx {
class Mesh {
  std::unique_ptr<BaseBuffer> buffer;
public:
  Mesh(Engine* engine, const char* path);
  
  void draw(BaseDevice* device);
};
}