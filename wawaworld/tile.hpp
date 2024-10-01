#pragma once
#include <glm/glm.hpp>
#include <mutex>

#include "gfx/base_types.hpp"
#include "gfx/engine.hpp"

namespace rdm {
enum Direction { North, South, West, East };

struct Tile {
  enum Type {
    DIRT_WALL,
    DIRT_FLOOR,
  } type;
};

struct Tilemap {
  std::mutex mutex;

  std::vector<Tile> tiles;
  int width;
  int height;

  /**
   * @brief called on tilemap modification
   *
   * please call it
   *
   * arg 1 is center, arg 2 is range
   */
  Signal<glm::vec2, int> modified;
};

class TilemapViewer : public gfx::Entity {
  glm::vec4 bounds;
  gfx::Engine* engine;
  Tilemap* map;
  bool dirty;

  void modifiedSignal(Tilemap* map, glm::vec2 center, int range);
  virtual void renderTechnique(gfx::BaseDevice* device, int id);

 public:
  TilemapViewer(gfx::Engine* engine, Graph::Node* node);

  void setTilemap(Tilemap* map);
  void setCenter(glm::vec2 p);

  void setDirty() { dirty = true; }
};
}  // namespace rdm
