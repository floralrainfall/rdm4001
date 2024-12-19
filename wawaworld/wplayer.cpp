#include "wplayer.hpp"

#include <cmath>
#include <memory>

#include "SDL_keycode.h"
#include "gfx/base_types.hpp"
#include "gfx/engine.hpp"
#include "gfx/imgui/imgui.h"
#include "gfx/material.hpp"
#include "gfx/mesh.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "network/entity.hpp"
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

std::map<int, std::string> WPlayer::weaponIds = {
    {1, "WeaponSniper"},
    {2, "WeaponMagnum"},
};

WPlayer::WPlayer(net::NetworkManager* manager, net::EntityId id)
    : Player(manager, id) {
  controller.reset(
      new rdm::putil::FpsController(manager->getWorld()->getPhysicsWorld()));
  controller->setLocalPlayer(false);
  entityNode = new rdm::Graph::Node();
  entityNode->scale = glm::vec3(6.f);
  wantedWeaponId = getManager()->isBackend() ? -1 : 1;
  heldWeaponRef = NULL;

  firingState[0] = false;
  firingState[1] = false;
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

    getManager()->addPendingUpdate(getEntityId());

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

        // if (heldWeaponRef) heldWeaponRef->renderWorld();
      } else {
        getGame()->getSoundManager()->listenerNode = entityNode;
        // if (heldWeaponRef) heldWeaponRef->renderView();
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
  } else {
    Worldspawn* worldspawn =
        dynamic_cast<Worldspawn*>(getManager()->findEntityByType("Worldspawn"));
    if (worldspawn) {
      controller->teleport(worldspawn->spawnLocation());
      getManager()->addPendingUpdateUnreliable(getEntityId());
    }

    giveWeapon(
        dynamic_cast<Weapon*>(getManager()->instantiate("WeaponSniper")));
  }
}

WPlayer::~WPlayer() {
  if (!getManager()->isBackend()) {
    getGfxEngine()->renderStepped.removeListener(gfxJob);
    getWorld()->stepped.removeListener(worldJob);
  }
}

void WPlayer::giveWeapon(Weapon* weapon) {
  if (getManager()->isBackend()) {
    weapon->setOwnerRef(this);
    ownedWeapons.push_back(weapon);
    getManager()->addPendingUpdate(getEntityId());
  }
}

void WPlayer::tick() {
  Worldspawn* worldspawn =
      dynamic_cast<Worldspawn*>(getManager()->findEntityByType("Worldspawn"));

  bool needsUpdate = false;

  if (!isLocalPlayer()) {
    if (heldWeaponRef) {
      if (firingState[0])
        heldWeaponRef->primaryFire();
      else if (firingState[1])
        heldWeaponRef->secondaryFire();
    }
  }

  if (!getManager()->isBackend()) {
    if (isLocalPlayer()) {
      for (int i = 0; i < 9; i++) {
        if (rdm::Input::singleton()->isKeyDown(SDLK_1 + i)) {
          Log::printf(LOG_DEBUG, "%i", i);
          auto it = weaponIds.find(i + 1);
          if (it != weaponIds.end()) {
            wantedWeaponId = i + 1;
            getManager()->addPendingUpdate(getEntityId());
          }
          break;
        }
      }

      if (rdm::Input::singleton()->isMouseButtonDown(1)) {
        if (heldWeaponRef) {
          if (!firingState[0]) needsUpdate = true;
          firingState[0] = true;
          heldWeaponRef->primaryFire();
        }
      } else {
        if (firingState[0]) needsUpdate = true;
        firingState[0] = false;
      }
    }
  }

  if (worldspawn && worldspawn->getFile()) {
    controller->setEnable(true);
    if (!getManager()->isBackend() && isLocalPlayer()) {
      btTransform transform = controller->getTransform();

      btVector3 vel = controller->getRigidBody()->getLinearVelocity();
      soundEmitter->setPitch(
          controller->isGrounded() ? ((vel.length() < 1) ? 0.0 : 1.f) : 0.f);

      if (worldspawn && worldspawn->getFile()) {
        btVector3 forward = btVector3(0, 0, 1);
        btVector3 forward_old = forward * transform.getBasis();
        btVector3 forward_new = forward * oldTransform.getBasis();
        btVector3 origin_old = transform.getOrigin();
        btVector3 origin_new = oldTransform.getOrigin();
        // Log::printf(LOG_DEBUG, "%f, %f", forward_old.dot(forward_new),
        //            origin_old.distance(origin_new));

        if (forward_old.dot(forward_new) < 0.9) needsUpdate = true;
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
  } else
    controller->setEnable(false);
}

void WPlayer::serialize(net::BitStream& stream) {
  Player::serialize(stream);
  if (getManager()->isBackend()) {
    stream.write<unsigned char>(heldWeaponIndex);
    stream.write<int>(ownedWeapons.size());
    for (int i = 0; i < ownedWeapons.size(); i++) {
      stream.write<network::EntityId>(ownedWeapons[i]->getEntityId());
    }
  } else {
    stream.write<unsigned char>(wantedWeaponId);
  }
}

void WPlayer::deserialize(net::BitStream& stream) {
  Player::deserialize(stream);
  if (getManager()->isBackend()) {
    unsigned char wantedWeapon = stream.read<unsigned char>();
    /*if (wantedWeaponId != wantedWeapon) {
      auto it = weaponIds.find(wantedWeapon);
      if (it != weaponIds.end()) {
        if (heldWeaponRef) {
          getManager()->deleteEntity(heldWeaponRef->getEntityId());
        }
        heldWeaponRef =
            dynamic_cast<Weapon*>(getManager()->instantiate(it->second));
        Log::printf(LOG_DEBUG, "Instantiated weapon %s",
                    heldWeaponRef->getTypeName());
        wantedWeaponId = wantedWeapon;
        heldWeaponId = heldWeaponRef->getEntityId();
        getManager()->addPendingUpdate(getEntityId());
      } else {
        Log::printf(LOG_WARN, "Unknown wantedWeapon id %i", wantedWeapon);
      }
      }*/
    if (ownedWeapons.size()) {
      if (wantedWeapon < ownedWeapons.size()) {
        heldWeaponRef = ownedWeapons[wantedWeapon];
        wantedWeaponId = wantedWeapon;
      } else {
        wantedWeaponId = 0;
        heldWeaponRef = ownedWeapons[wantedWeaponId];
      }
    } else {
      heldWeaponRef = 0;
    }
  } else {
    wantedWeaponId = stream.read<unsigned char>();
    heldWeaponIndex = wantedWeaponId;
    ownedWeapons.clear();
    int numWeapons = stream.read<int>();
    for (int i = 0; i < numWeapons; i++) {
      net::EntityId id = stream.read<net::EntityId>();
      Weapon* weapon = dynamic_cast<Weapon*>(getManager()->getEntityById(id));
      if (weapon) {
        weapon->setOwnerRef(this);
        ownedWeapons.push_back(weapon);
      } else {
        Log::printf(LOG_ERROR, "Invalid weapon id %i", id);
      }
    }
    if (ownedWeapons.size()) {
      heldWeaponRef = ownedWeapons[wantedWeaponId];
    }
  }
}

void WPlayer::serializeUnreliable(net::BitStream& stream) {
  {
    std::scoped_lock lock(getWorld()->getPhysicsWorld()->mutex);
    controller->serialize(stream);
  }

  stream.write<bool>(firingState[0]);
  stream.write<bool>(firingState[1]);
}

void WPlayer::deserializeUnreliable(net::BitStream& stream) {
  {
    std::scoped_lock lock(getWorld()->getPhysicsWorld()->mutex);
    controller->deserialize(stream, getManager()->isBackend());
  }

  if (!isLocalPlayer()) {
    firingState[0] = stream.read<bool>();
    firingState[1] = stream.read<bool>();
  }
}
}  // namespace ww
