#pragma once
#include <cstring>
#include <string>

namespace common {
struct Guid {
  unsigned char guid[16];

  static Guid generateGuid();

  std::string toString();
  bool operator==(const Guid& rhs) const {
    return (memcmp(guid, rhs.guid, 16) == 0);
  }
};
}  // namespace common