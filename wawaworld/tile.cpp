#include "tile.hpp"
namespace rdm {
TilemapViewer::TilemapViewer(gfx::Engine* engine, Graph::Node* node)
    : Entity(node) {
  dirty = true;
  map = 0;
}

void TilemapViewer::renderTechnique(gfx::BaseDevice* device, int id) {
  if (dirty) {
    if (!map) return;

    dirty = false;
  }
}

void TilemapViewer::modifiedSignal(Tilemap* map, glm::vec2 center, int range) {
  if (this->map != map) return;
  std::scoped_lock lock(map->mutex);

  glm::vec2 b1l = glm::vec2(bounds.x, bounds.y);
  glm::vec2 b1r = glm::vec2(bounds.x + bounds.z, bounds.y + bounds.w);
  glm::vec2 b2l = glm::vec2(center.x - (range / 2), center.y - (range / 2));
  glm::vec2 b2r = glm::vec2(center.x + (range / 2), center.y + (range / 2));

  if (b1l.x > b2r.x || b2l.x > b1r.x) return;
  if (b1r.y > b2l.y || b2r.y > b1l.y) return;

  dirty = true;
}

void TilemapViewer::setCenter(glm::vec2 p) {
  bounds.x = p.x - (bounds.z / 2);
  bounds.y = p.y - (bounds.w / 2);
  setDirty();
}

void TilemapViewer::setTilemap(Tilemap* map) {
  map->modified.listen([this, map](glm::vec2 center, int range) {
    modifiedSignal(map, center, range);
  });
  this->map = map;
}
}  // namespace rdm
