#pragma once
#include "engine.hpp"
#include "entity.hpp"
namespace rdm::gfx {
class HeightmapEntity : public Entity {
  std::unique_ptr<BaseBuffer> elementBuffer;
  std::unique_ptr<BaseBuffer> arrayBuffer;
  std::unique_ptr<BaseArrayPointers> arrayPointers;
  int elementCount;

  virtual void renderTechnique(BaseDevice* device, int id);

 public:
  HeightmapEntity(Graph::Node* node);
  void generateMesh(Engine* engine);
};
};  // namespace rdm::gfx