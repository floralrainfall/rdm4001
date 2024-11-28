#include "wplayer.hpp"

#include <cmath>
#include <memory>

#include "gfx/base_types.hpp"
#include "gfx/engine.hpp"
#include "gfx/imgui/imgui.h"
#include "gfx/material.hpp"
#include "gfx/mesh.hpp"
#include "logging.hpp"
#include "physics.hpp"
#include "putil/fpscontroller.hpp"
#include "settings.hpp"
#include "sound.hpp"
#include "wgame.hpp"
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

static CVar cl_showpos("cl_showpos", "0", CVARF_SAVE);

WPlayer::WPlayer(net::NetworkManager* manager, net::EntityId id)
    : Player(manager, id) {
  controller.reset(
      new rdm::putil::FpsController(manager->getWorld()->getPhysicsWorld()));
  controller->setLocalPlayer(false);
  entityNode = new rdm::Graph::Node();
  entityNode->scale = glm::vec3(6.f);
  if (!getManager()->isBackend()) {
    soundEmitter.reset(getGame()->getSoundManager()->newEmitter());
    soundEmitter->node = entityNode;
    soundEmitter->play(getGame()
                           ->getSoundManager()
                           ->getSoundCache()
                           ->get("dat5/walking.ogg")
                           .value());
    soundEmitter->setPitch(0.f);
    soundEmitter->setLooping(true);

    worldJob = getWorld()->stepped.listen([this] {
      if (isLocalPlayer()) {
        controller->updateCamera(getGfxEngine()->getCamera());
      }
    });
    gfxJob = getGfxEngine()->renderStepped.listen([this] {
      {
        std::scoped_lock lock(getWorld()->getPhysicsWorld()->mutex);
        btTransform transform;
        controller->getMotionState()->getWorldTransform(transform);
        entityNode->origin =
            rdm::BulletHelpers::fromVector3(transform.getOrigin());
        entityNode->basis = rdm::BulletHelpers::fromMat3(transform.getBasis()) *
                            glm::mat3(glm::rotate(M_PI_2f, glm::vec3(0, 0, 1)));
      }

      if (isLocalPlayer() && cl_showpos.getBool()) {
        Worldspawn* worldspawn = dynamic_cast<Worldspawn*>(
            getManager()->findEntityByType("Worldspawn"));
        ImGui::Begin("Debug");
        if (worldspawn && worldspawn->getFile()) {
          ImGui::Text("Cluster: %i", worldspawn->getFile()->getVisCluster());
          ImGui::Text("Rendered Faces: %i",
                      worldspawn->getFile()->getFacesRendered());
          ImGui::Text("Rendered Leafs: %i",
                      worldspawn->getFile()->getLeafsRendered());

          btVector3 vel = controller->getRigidBody()->getLinearVelocity();
          ImGui::Text("Velocity: %0.2f, %0.2f, %0.2f", vel.x(), vel.y(),
                      vel.z());
          ImGui::Text("Velocity Length: %0.2f", vel.length());
          glm::vec2 accel = controller->getWishDir();
          ImGui::Text("Wish Dir: %0.2f, %0.2f", accel.x, accel.y);
        } else {
          ImGui::Text("No worldspawn found/map not loaded");
        }

        btTransform transform = controller->getTransform();
        btVector3 origin = transform.getOrigin();
        ImGui::Text("Position: %0.2f, %0.2f, %0.2f", origin.x(), origin.y(),
                    origin.z());
        ImGui::End();
      }

      // if (getManager()->getLocalPeer().peerId == remotePeerId.get()) return;
      getGame()->getSoundManager()->listenerNode = entityNode;

      if (!isLocalPlayer()) {
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

        entityNode->origin = controller->getNetworkPosition();
        program->setParameter("model", gfx::DtMat4,
                              gfx::BaseProgram::Parameter{
                                  .matrix4x4 = entityNode->worldTransform()});
        model->render(getGfxEngine()->getDevice());
      }
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
  Worldspawn* worldspawn =
      dynamic_cast<Worldspawn*>(getManager()->findEntityByType("Worldspawn"));

  if (worldspawn && worldspawn->getFile()) {
    controller->setEnable(true);
    if (!getManager()->isBackend()) {
      btTransform transform = controller->getTransform();

      btVector3 vel = controller->getRigidBody()->getLinearVelocity();
      soundEmitter->setPitch(
          controller->isGrounded() ? ((vel.length() < 1) ? 0.0 : 1.f) : 0.f);

      if (isLocalPlayer()) {
        if (worldspawn && worldspawn->getFile()) {
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
        }

        controller->setLocalPlayer(true);
      } else {
        controller->setLocalPlayer(false);
      }
    }
  } else
    controller->setEnable(false);
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
