#pragma once
#include <functional>
namespace rdm {
class Game;

class GameState {
  Game* game;
  float timer;

 public:
  GameState(Game* game);

  enum States {
    Intro,
    MainMenu,
  };

 private:
  States state;
};

template <typename T>
GameState* GameStateConstructor(Game* game) {
  return new T(game);
}
typedef std::function<GameState*(Game* game)> GameStateConstructorFunction;
};  // namespace rdm
