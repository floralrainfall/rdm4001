#pragma once
#include "game.hpp"

namespace ww {
using namespace rdm;
struct WGamePrivate;
class WGame : public Game {
  WGamePrivate* game;

 public:
  WGame();
  ~WGame();

  virtual void initialize();
  virtual void initializeClient();
};
};  // namespace ww
