#include "guid.hpp"
#include <iomanip>
#include <format>
#include "random.hpp"

namespace common {
  Guid Guid::generateGuid() {
    
    Guid r;
    for(int i = 0; i < 16; i++)
      r.guid[i] = Random::singleton()->random8();
    return r;
  }
  
  std::string Guid::toString() {
    std::string r = "";
    for(int i = 0; i < 16; i++) {
      char x[3];
      snprintf(x,3,"%02x",guid[i]); // Sorry
      r += std::string(x);
    }
    return r;
  }
}