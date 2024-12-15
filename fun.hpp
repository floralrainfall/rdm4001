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

  static glm::vec4 stringToVec4(std::string str) {
    glm::vec4 v(0);
    size_t pos = 0;
    std::string s = str;
    std::string token;
    enum pos_t {
      POSITION_X,
      POSITION_Y,
      POSITION_Z,
      POSITION_W
    }* pos_e = (pos_t*)&pos;
    int c = 0;
    while ((pos = s.find(" ")) != std::min(std::string::npos, (size_t)4)) {
      std::string token = s.substr(0, pos);
      switch ((pos_t)c) {
        case POSITION_X:
          v.x = std::stof(token);
          break;
        case POSITION_Y:
          v.y = std::stof(token);
          break;
        case POSITION_Z:
          v.z = std::stof(token);
          break;
        case POSITION_W:
          v.w = std::stof(token);
          break;
      }
      s.erase(0, pos + 1);
      c++;
      if (c == 4) break;
    }
    return v;
  }
};
}  // namespace rdm
