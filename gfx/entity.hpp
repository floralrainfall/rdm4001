#pragma once
#include "base_device.hpp"
#include "graph.hpp"
#include "material.hpp"

namespace rdm::gfx {
class Entity {
  Graph::Node* node;
  std::shared_ptr<Material> material;

  bool enableRender;

 protected:
  virtual void renderTechnique(BaseDevice* device, int id) = 0;

 public:
  Entity(Graph::Node* node);

  virtual void initialize() {};
  virtual ~Entity() {};

  void render(BaseDevice* device);

  bool canRender() { return enableRender; }
  void setCanRender(bool s) { enableRender = s; }

  void setMaterial(std::shared_ptr<Material> material) {
    this->material = material;
  };
};

class BufferEntity : public Entity {
  std::unique_ptr<BaseBuffer> buff;
  std::unique_ptr<BaseArrayPointers> arrayPointers;
  int count;

  virtual void renderTechnique(BaseDevice* device, int id);

 public:
  BufferEntity(std::unique_ptr<BaseBuffer> buff, int count,
               std::unique_ptr<BaseArrayPointers> arrayPointers,
               Graph::Node* node);
};
}  // namespace rdm::gfx
