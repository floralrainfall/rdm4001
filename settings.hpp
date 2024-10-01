#pragma once
#include <map>
#include <string>
#include <variant>

#include "json.hpp"
using json = nlohmann::json;

namespace rdm {
typedef std::variant<std::string, int, float> Setting;
class Settings {
  Settings();

  std::map<std::string, json> settings;
  std::string settingsPath;

 public:
  static Settings* singleton();

  void parseCommandLine(char* argv[], int argc);
  void load();
  void save();

  json getSetting(std::string setting, json unset = json());
};
}  // namespace rdm