#pragma once
#include <string>
namespace rdm {
class Fun {
 public:
  static bool preFlightChecks();
  static std::string getModuleName();
};
}  // namespace rdm
