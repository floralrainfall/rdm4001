#include "video.hpp"
namespace rdm::gfx {
VideoRenderer::VideoRenderer(gfx::Engine* engine) { this->engine = engine; }

VideoRenderer::~VideoRenderer() {}

void VideoRenderer::play(const char* path) {}

void VideoRenderer::render() {}
}  // namespace rdm::gfx
