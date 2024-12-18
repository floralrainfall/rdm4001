#pragma once

#include "gfx/entity.hpp"
#include "graph.hpp"
#include "network/bitstream.hpp"
#include "network/entity.hpp"
#include "network/player.hpp"
#include "putil/fpscontroller.hpp"
#include "signal.hpp"
#include "sound.hpp"
#include "weapon.hpp"

namespace net = rdm::network;
namespace ww {

class WPlayer : public net::Player {
  std::unique_ptr<rdm::putil::FpsController> controller;
  rdm::gfx::Entity* entity;
  rdm::Graph::Node* entityNode;
  rdm::ClosureId worldJob;
  rdm::ClosureId gfxJob;
  btTransform oldTransform;
  int wantedWeaponId;
  net::EntityId heldWeaponId;
  Weapon* heldWeaponRef;

  static std::map<int, std::string> weaponIds;

  int health;
  int maxHealth;
  int armor;
  int maxArmor;

  bool firingState[2];

  std::unique_ptr<rdm::SoundEmitter> soundEmitter;

 public:
  WPlayer(net::NetworkManager* manager, net::EntityId id);
  virtual ~WPlayer();

  rdm::Graph::Node* getNode() { return entityNode; }

  virtual void tick();
  rdm::putil::FpsController* getController() { return controller.get(); }

  virtual void serialize(net::BitStream& stream);
  virtual void deserialize(net::BitStream& stream);

  virtual void serializeUnreliable(net::BitStream& stream);
  virtual void deserializeUnreliable(net::BitStream& stream);
  virtual const char* getTypeName() { return "WPlayer"; };
};
};  // namespace ww
