#pragma once
#include "america.hpp"
#include "network/entity.hpp"
#include "network/network.hpp"
#include "network/player.hpp"
#include "signal.hpp"
namespace rt {
class Pawn : public rdm::network::Player {
  America::Location location;
  America::Location desiredLocation;
  rdm::ClosureId gfxTick;

  int cash;
  bool turnEnded;
  bool vacationed;

 public:
  Pawn(rdm::network::NetworkManager* manager, rdm::network::EntityId id);

  bool isTurnDone() { return turnEnded; }
  void endTurn();

  virtual void tick();

  virtual void serialize(rdm::network::BitStream& stream);
  virtual void deserialize(rdm::network::BitStream& stream);

  virtual ~Pawn();
  virtual const char* getTypeName() { return "Pawn"; }
};
}  // namespace rt
