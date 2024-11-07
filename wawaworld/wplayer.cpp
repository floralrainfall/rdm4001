#include "wplayer.hpp"

#include <cmath>
#include <memory>

#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btVector3.h"
#include "gfx/base_types.hpp"
#include "gfx/engine.hpp"
#include "gfx/imgui/imgui.h"
#include "gfx/material.hpp"
#include "gfx/mesh.hpp"
#include "logging.hpp"
#include "physics.hpp"
#include "putil/fpscontroller.hpp"
#include "world.hpp"
#include "worldspawn.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace gfx = rdm::gfx;
namespace ww {
class PlayerEntity : public gfx::Entity {
  WPlayer* player;
  gfx::Model* playerModel;

 public:
  PlayerEntity(WPlayer* player, gfx::Model* model, rdm::Graph::Node* node)
      : gfx::Entity(node) {
    this->player = player;
    this->playerModel = model;
  }
  virtual void renderTechnique(gfx::BaseDevice* device, int id) {
    playerModel->render(device);
  }
};

WPlayer::WPlayer(net::NetworkManager* manager, net::EntityId id)
    : Player(manager, id) {
  controller.reset(
      new rdm::putil::FpsController(manager->getWorld()->getPhysicsWorld()));
  entityNode = new rdm::Graph::Node();
  entityNode->scale = glm::vec3(6.f);
  if (!getManager()->isBackend()) {
    worldJob = getWorld()->stepped.listen([this] {
      if (getManager()->getLocalPeer().peerId == remotePeerId.get()) {
        controller->updateCamera(getGfxEngine()->getCamera());
      }
    });
    gfxJob = getGfxEngine()->renderStepped.listen([this] {
      {
        std::scoped_lock lock(getWorld()->getPhysicsWorld()->mutex);
        btTransform transform = controller->getTransform();
        entityNode->origin =
            rdm::BulletHelpers::fromVector3(transform.getOrigin());
        entityNode->basis = rdm::BulletHelpers::fromMat3(transform.getBasis()) *
                            glm::mat3(glm::rotate(M_PI_2f, glm::vec3(0, 0, 1)));
      }
      std::shared_ptr<gfx::Material> material =
          getGfxEngine()->getMaterialCache()->getOrLoad("Mesh").value();
      gfx::BaseProgram* program =
          material->prepareDevice(getGfxEngine()->getDevice(), 0);
      program->setParameter("model", gfx::DtMat4,
                            gfx::BaseProgram::Parameter{
                                .matrix4x4 = entityNode->worldTransform()});
      gfx::Model* model = getGfxEngine()
                              ->getMeshCache()
                              ->get("dat5/baseq3/models/andi_rig.obj")
                              .value();
      model->render(getGfxEngine()->getDevice());
    });
    /*getGfxEngine()->renderStepped.addClosure([this] {
      gfx::Entity* ent = getGfxEngine()->addEntity<PlayerEntity>(
          this,
          entityNode);
      ent->setMaterial(
          getGfxEngine()->getMaterialCache()->getOrLoad("Mesh").value());
          });*/
    rdm::Log::printf(rdm::LOG_DEBUG, "worldJob = %i", worldJob);
  }
}

WPlayer::~WPlayer() {
  if (!getManager()->isBackend()) {
    getGfxEngine()->renderStepped.removeListener(gfxJob);
    getWorld()->stepped.removeListener(worldJob);
  }
}

void WPlayer::tick() {
  if (!getManager()->isBackend()) {
    btTransform transform = controller->getTransform();

    if (getManager()->getLocalPeer().peerId == remotePeerId.get()) {
      controller->setLocalPlayer(true);
      btVector3 forward = btVector3(0, 0, 1);
      btVector3 forward_old = forward * transform.getBasis();
      btVector3 forward_new = forward * oldTransform.getBasis();
      btVector3 origin_old = transform.getOrigin();
      btVector3 origin_new = oldTransform.getOrigin();
      bool needsUpdate = false;
      if (forward_old.dot(forward_new) > 0.1) needsUpdate = true;
      if (origin_old.distance(origin_new) > 0.1) needsUpdate = true;

      if (needsUpdate) {
        oldTransform = transform;

        getManager()->addPendingUpdateUnreliable(getEntityId());
      }

      getGfxEngine()->renderStepped.addClosure([=] {
        ImGui::Begin("Debug");
        Worldspawn* worldspawn = dynamic_cast<Worldspawn*>(
            getManager()->findEntityByType("Worldspawn"));
        if (worldspawn) {
          ImGui::Text("Cluster: %i", worldspawn->getFile()->getVisCluster());
          ImGui::Text("Rendered Faces: %i",
                      worldspawn->getFile()->getFacesRendered());
          ImGui::Text("Rendered Leafs: %i",
                      worldspawn->getFile()->getLeafsRendered());
        } else {
          ImGui::Text("No worldspawn found");
        }
        ImGui::End();
      });
    } else {
      controller->setLocalPlayer(false);
    }
  }
}

void WPlayer::serialize(net::BitStream& stream) { Player::serialize(stream); }

void WPlayer::deserialize(net::BitStream& stream) {
  Player::deserialize(stream);
}

void WPlayer::serializeUnreliable(net::BitStream& stream) {
  std::scoped_lock lock(getWorld()->getPhysicsWorld()->mutex);
  controller->serialize(stream);
}

void WPlayer::deserializeUnreliable(net::BitStream& stream) {
  std::scoped_lock lock(getWorld()->getPhysicsWorld()->mutex);
  controller->deserialize(stream);
}
}  // namespace ww
