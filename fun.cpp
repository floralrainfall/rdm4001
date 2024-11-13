#include "fun.hpp"

#include <stdio.h>
#include <stdlib.h>

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
}  // namespace rdm
