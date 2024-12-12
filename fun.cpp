#include "fun.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <format>
#include <fstream>
#include <iostream>

#include "logging.hpp"
#include "settings.hpp"
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

static CVar cl_username("cl_username", "", CVARF_GLOBAL | CVARF_SAVE);

std::string Fun::getSystemUsername() {
  std::string username;
  if (cl_username.getValue().empty()) {
#ifdef __linux
    char* _username = getenv("USER");
    username = _username;
#else
    // do it for windows
    username = std::format("User{}", rand() % 99999);
#endif
    cl_username.setValue(username);
  } else {
    username = cl_username.getValue();
  }
  return username;
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
