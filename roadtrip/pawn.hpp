#pragma once
#include "america.hpp"
#include "network/entity.hpp"
#include "network/network.hpp"
#include "network/player.hpp"
#include "signal.hpp"
namespace rt {
struct PawnAction {
  enum {
    Buy,
    Use,
  } type;
  union {
    struct {
    } buy;
  } data;
};

class Pawn : public rdm::network::Player {
  America::Location location;
  America::Location desiredLocation;
  America::Location caPointOfEntry;
  bool inCanada;
  rdm::ClosureId gfxTick;

  int cash;
  bool turnEnded;
  bool vacationed;

 public:
  Pawn(rdm::network::NetworkManager* manager, rdm::network::EntityId id);

  bool isTurnDone() { return turnEnded; }
  bool isVacationed() { return vacationed; }
  void endTurn();

  virtual void tick();

  virtual void serialize(rdm::network::BitStream& stream);
  virtual void deserialize(rdm::network::BitStream& stream);

  virtual ~Pawn();
  virtual const char* getTypeName() { return "Pawn"; }
};
}  // namespace rt
