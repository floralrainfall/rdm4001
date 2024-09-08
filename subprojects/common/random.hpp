#pragma once
#include <stdint.h>
#include <random>

namespace common {
  // should be a random enough system
  // game should use RNG
  class Random {
    std::random_device dev;
    std::mt19937 mersenne;

    Random();
  public:
    static Random* singleton();

    uint8_t random8();
  };
}