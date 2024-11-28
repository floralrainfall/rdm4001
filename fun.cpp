#include "fun.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>

#include "logging.hpp"
namespace rdm {
bool Fun::preFlightChecks() {
#ifdef __linux
  FILE* fp = fopen("/proc/sys/fs/binfmt_misc/WSLInterop", "r");
  if (fp) {
    printf(
        "Your graphics card is not supported by RDM4001, and thus it cannot "
        "start. Sorry\n");
    return false;
  }
#endif
  return true;  // OK
}

std::string Fun::getModuleName() {
#if defined(PLATFORM_POSIX) || defined(__linux__)
  std::string sp;
  std::ifstream("/proc/self/comm") >> sp;
  return sp;
#elif defined(_WIN32
  char buf[MAX_PATH];
  GetModuleFileNameA(nullptr, buf, MAX_PATH);
  return buf;
#else
  static_assert(false, "unrecognized platform");
#endif
}
}  // namespace rdm
