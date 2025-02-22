#pragma once
#include "base_device.hpp"
#include "base_types.hpp"
namespace rdm::gfx {
class Engine;

class VideoRenderer {
  std::unique_ptr<BaseTexture> videoTexture;
  Engine* engine;

 public:
  VideoRenderer(Engine* engine);
  ~VideoRenderer();

  void play(const char* path);
  void render();

  enum Status { Playing, Finished, Paused };

  Status getStatus() { return currentStatus; };

 private:
  Status currentStatus;
};
}  // namespace rdm::gfx
