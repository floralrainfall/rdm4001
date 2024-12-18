#include "weapon.hpp"

#include "gfx/engine.hpp"
#include "gfx/mesh.hpp"
#include "wplayer.hpp"
namespace ww {
Weapon::Weapon(net::NetworkManager* manager, net::EntityId id)
    : net::Entity(manager, id) {
  ownerRef = NULL;
}

void Weapon::primaryFire() {}

void Weapon::secondaryFire() {}

void Weapon::renderView() {
  if (viewModel.empty()) return;
  if (!ownerRef) return;

  using namespace rdm;

  Graph::Node node;
  node.parent = ownerRef->getNode();
  node.origin = glm::vec3(-1, -2, 2);

  std::shared_ptr<gfx::Material> material =
      getGfxEngine()->getMaterialCache()->getOrLoad("Mesh").value();
  gfx::BaseProgram* program =
      material->prepareDevice(getGfxEngine()->getDevice(), 0);
  program->setParameter(
      "model", gfx::DtMat4,
      gfx::BaseProgram::Parameter{.matrix4x4 = node.worldTransform()});
  gfx::Model* model =
      getGfxEngine()->getMeshCache()->get(viewModel.c_str()).value();
  model->render(getGfxEngine()->getDevice());
}

void Weapon::renderWorld() {}
};  // namespace ww
