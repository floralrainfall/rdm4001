#include "random.hpp"

namespace common {
  Random::Random() : mersenne(dev()) {

  }

  static Random* _singleton = 0;
  Random* Random::singleton() {
    if(!_singleton)
      _singleton = new Random();
    return _singleton;
  }

  uint8_t Random::random8() {
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, 255);
    return (uint8_t)dist(mersenne);
  }
}