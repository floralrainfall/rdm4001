#pragma once

#include "network/bitstream.hpp"
#include "network/entity.hpp"
#include "network/player.hpp"
#include "putil/fpscontroller.hpp"
namespace net = rdm::network;
namespace ww {
class WPlayer : public net::Player {
  std::unique_ptr<rdm::putil::FpsController> controller;

 public:
  WPlayer(net::NetworkManager* manager, net::EntityId id);

  virtual void tick();

  virtual void serialize(net::BitStream& stream);
  virtual void deserialize(net::BitStream& stream);

  virtual void serializeUnreliable(net::BitStream& stream);
  virtual void deserializeUnreliable(net::BitStream& stream);
  virtual const char* getTypeName() { return "WPlayer"; };
};
};  // namespace ww