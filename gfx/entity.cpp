#include "entity.hpp"

#include <stdexcept>

#include "gfx/base_types.hpp"

namespace rdm::gfx {
Entity::Entity(Graph::Node* node) {
  this->node = node;
  enableRender = true;
}

void Entity::render(BaseDevice* device) {
  int numTechniques = 1;
  if (material) numTechniques = material->numTechniques();
  for (int i = 0; i < numTechniques; i++) {
    if (material) {
      BaseProgram* program = material->prepareDevice(device, i);
      if (node) {
        program->setParameter(
            "model", DtMat4,
            BaseProgram::Parameter{.matrix4x4 = node->worldTransform()});
      }
    }
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
