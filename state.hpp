#pragma once
#include <functional>

#include "sound.hpp"
namespace rdm {
class Game;

class GameState {
  Game* game;
  float timer;
  rdm::SoundEmitter* emitter;

 public:
  GameState(Game* game);

  enum States {
    Intro,
    MainMenu,
    MenuOnlinePlay,
    InGame,
  };

  std::map<States, std::string> stateMusic;

 private:
  States state;
};

template <typename T>
GameState* GameStateConstructor(Game* game) {
  return new T(game);
}
typedef std::function<GameState*(Game* game)> GameStateConstructorFunction;
};  // namespace rdm
