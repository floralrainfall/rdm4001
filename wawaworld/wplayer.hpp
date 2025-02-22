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
  btVector3 oldFront;
  int wantedWeaponId;
  Weapon* heldWeaponRef;
  int heldWeaponIndex;
  std::vector<Weapon*> ownedWeapons;

  static std::map<int, std::string> weaponIds;

  int health;
  int maxHealth;
  int armor;
  int maxArmor;

  bool firingState[2];

  std::unique_ptr<rdm::SoundEmitter> soundEmitter;

 public:
  enum Status { Spectator, InGame };

  WPlayer(net::NetworkManager* manager, net::EntityId id);
  virtual ~WPlayer();

  void listWeapons();

  void giveWeapon(Weapon* weapon);

  rdm::Graph::Node* getNode() { return entityNode; }

  virtual void tick();

  rdm::putil::FpsController* getController() { return controller.get(); }

  virtual std::string getEntityInfo();

  virtual void serialize(net::BitStream& stream);
  virtual void deserialize(net::BitStream& stream);

  virtual void serializeUnreliable(net::BitStream& stream);
  virtual void deserializeUnreliable(net::BitStream& stream);
  virtual const char* getTypeName() { return "WPlayer"; };

  void setStatus(Status status) {
    this->status = status;
    getManager()->addPendingUpdate(getEntityId());
  };
  Status getStatus() { return status; };

 private:
  Status status;
};
};  // namespace ww
