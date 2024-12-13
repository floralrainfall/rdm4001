#include "gstate.hpp"

#include "game.hpp"
#include "state.hpp"
namespace ww {
WWGameState::WWGameState(rdm::Game* game) : rdm::GameState(game) {
  // stateMusic[MainMenu] = "dat5/mus/main_menu.ogg";
}
}  // namespace ww
