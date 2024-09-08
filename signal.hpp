#pragma once
#include <functional>

namespace rdm {
  template<typename ...Args>
  class Signal {
    std::vector<std::function<void(Args...)>> listeners;
  public:
    void fire(Args... a) {
      for(auto listener : listeners) {
        listener(a...);
      }
    };

    void listen(std::function<void(Args...)> a) {
      listeners.push_back(a);
    }
  };
}