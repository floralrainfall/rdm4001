#pragma once

#include "gfx/entity.hpp"
#include "graph.hpp"
#include "network/bitstream.hpp"
#include "network/entity.hpp"
#include "network/player.hpp"
#include "putil/fpscontroller.hpp"
#include "signal.hpp"
#include "sound.hpp"
namespace net = rdm::network;
namespace ww {
class WPlayer : public net::Player {
  std::unique_ptr<rdm::putil::FpsController> controller;
  rdm::gfx::Entity* entity;
  rdm::Graph::Node* entityNode;
  rdm::ClosureId worldJob;
  rdm::ClosureId gfxJob;
  btTransform oldTransform;

  std::unique_ptr<rdm::SoundEmitter> soundEmitter;

 public:
  WPlayer(net::NetworkManager* manager, net::EntityId id);
  virtual ~WPlayer();

  virtual void tick();

  virtual void serialize(net::BitStream& stream);
  virtual void deserialize(net::BitStream& stream);

  virtual void serializeUnreliable(net::BitStream& stream);
  virtual void deserializeUnreliable(net::BitStream& stream);
  virtual const char* getTypeName() { return "WPlayer"; };
};
};  // namespace ww
