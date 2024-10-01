#include "entity.hpp"

#include <stdexcept>

namespace rdm::gfx {
Entity::Entity(Graph::Node* node) {
  this->node = node;
  enableRender = true;
}

void Entity::render(BaseDevice* device) {
  if (!material) {
    throw std::runtime_error("No material specified for Entity");
  }
  for (int i = 0; i < material->numTechniques(); i++) {
    material->prepareDevice(device, i);
    renderTechnique(device, i);
  }
}

BufferEntity::BufferEntity(std::unique_ptr<BaseBuffer> buffer, int count,
                           std::unique_ptr<BaseArrayPointers> arrayPointers,
                           Graph::Node* node)
    : Entity(node) {
  this->buff = std::move(buffer);
  this->arrayPointers = std::move(arrayPointers);
  this->count = count;
}

void BufferEntity::renderTechnique(BaseDevice* device, int id) {
  arrayPointers->bind();
  device->draw(this->buff.get(), DtUnsignedByte, BaseDevice::Triangles, count);
}
}  // namespace rdm::gfx
