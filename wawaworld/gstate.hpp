#pragma once
#include "game.hpp"
#include "state.hpp"
namespace ww {
class WWGameState : public rdm::GameState {
 public:
  WWGameState(rdm::Game* game);
};
}  // namespace ww
