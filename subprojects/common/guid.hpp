#pragma once
#include <string>
#include <cstring>

namespace common {
  struct Guid {
    unsigned char guid[16];

    static Guid generateGuid();

    std::string toString();
    bool operator==(const Guid& rhs) const {
      return(memcmp(guid, rhs.guid, 16) == 0);
    }
  };
}