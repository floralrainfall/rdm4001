#pragma once
#include "network/entity.hpp"
namespace net = rdm::network;
namespace ww {
class WPlayer;
class Weapon : public net::Entity {
  WPlayer* ownerRef;
  int ammoPrimary;

 protected:
  std::string viewModel;
  std::string worldModel;

 public:
  Weapon(net::NetworkManager* manager, net::EntityId id);

  void renderWorld();
  void renderView();

  virtual void primaryFire();
  virtual void secondaryFire();

  int getAmmoPrimary() { return ammoPrimary; }
  void setOwnerRef(WPlayer* player) { ownerRef = player; };
  WPlayer* getOwnerRef() { return ownerRef; }

  virtual const char* getTypeName() { return "Weapon"; };
};
};  // namespace ww
