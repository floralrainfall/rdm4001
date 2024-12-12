#pragma once
#include <glm/glm.hpp>
#include <string>
namespace rdm {
class Fun {
 public:
  static bool preFlightChecks();
  static std::string getModuleName();
  static std::string getSystemUsername();
};

class Math {
 public:
  static bool pointInRect2d(glm::vec4 rect, glm::vec2 point) {
    return point.x >= rect.x && point.y >= rect.y &&
           point.x <= rect.x + rect.z && point.y <= rect.y + rect.w;
  }
};
}  // namespace rdm
