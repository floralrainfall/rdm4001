#pragma once
#include "game.hpp"

namespace rm {
using namespace rdm;
struct RGamePrivate;

class RGame : public Game {
  RGamePrivate* game;

 public:
  RGame();
  ~RGame();

  virtual void initialize();
};
}  // namespace rm
