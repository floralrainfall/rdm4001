#pragma once
#include <stdexcept>
#include "game.hpp"
#include "logging.hpp"

#define LAUNCHER_VERSION 0x0000001

namespace launcher {
  struct Exports {
    std::string name;
    std::string copyright;
    
    rdm::Game* (*init)(int);
  };

  template<typename T>
  rdm::Game* ExportInit(int version) {
    if(version != LAUNCHER_VERSION) {
      rdm::Log::printf(rdm::LOG_FATAL, "Bad launcher version (compiled with %i, received %i)", LAUNCHER_VERSION, version);
      throw std::runtime_error("Bad launcher version");
    }
    T* g = new T();
    return g;
  }
};

#define LAUNCHER_DEFINE_EXPORTS(T, N, C)				\
  extern launcher::Exports __RDM4001_LAUNCHER_EXPORTS = {		\
    .name = N,								\
    .copyright = C,							\
    .init = launcher::ExportInit<T>					\
  }						
