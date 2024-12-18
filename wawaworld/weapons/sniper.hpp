#pragma once
#include "../weapon.hpp"
#include "network/network.hpp"
#include "sound.hpp"
namespace ww {
class WeaponSniper : public Weapon {
  float nextPrimary;
  std::unique_ptr<rdm::SoundEmitter> emitter;

 public:
  WeaponSniper(net::NetworkManager* manager, net::EntityId id);

  virtual void tick();
  virtual void primaryFire();

  virtual const char* getTypeName() { return "WeaponSniper"; };
};
};  // namespace ww
