#include "settings.hpp"

#include <stdio.h>

#include <boost/program_options.hpp>
#include <format>
#include <iostream>
#include <string>

#include "json.hpp"
#include "logging.hpp"

namespace rdm {
Settings::Settings() { settingsPath = "settings.json"; }

CVar::CVar(const char* name, const char* defaultVar, unsigned long flags) {
  this->name = name;
  this->flags = flags;
  value = defaultVar;
  Settings::singleton()->addCvar(name, this);
}

void CVar::setValue(std::string s) {
  this->value = s;
  if (flags & CVARF_NOTIFY) Settings::singleton()->cvarChanging.fire(name);
  changing.fire();
}

int CVar::getInt() { return std::stoi(value); }

void CVar::setInt(int i) { setValue(std::to_string(i)); }

float CVar::getFloat() { return std::stof(value); }

void CVar::setFloat(float f) { setValue(std::to_string(f)); }

// taken from
// https://github.com/floralrainfall/matrix/blob/trunk/matrix/src/mcvar.cpp

bool CVar::getBool() { return value[0] != '0'; }

void CVar::setBool(bool b) { setValue(b ? "1" : "0"); }

glm::vec2 CVar::getVec2() {
  glm::vec4 v = getVec4();
  return glm::vec2(v.x, v.y);
}

void CVar::setVec2(glm::vec2 v) { setValue(std::format("{} {}", v.x, v.y)); }

glm::vec3 CVar::getVec3() {
  glm::vec4 v = getVec4();
  return glm::vec3(v.x, v.y, v.z);
}

void CVar::setVec3(glm::vec3 v) {
  setValue(std::format("{} {} {}", v.x, v.y, v.z));
}

glm::vec4 CVar::getVec4() {
  glm::vec4 v(0);
  size_t pos = 0;
  std::string s = value;
  std::string token;
  enum pos_t {
    POSITION_X,
    POSITION_Y,
    POSITION_Z,
    POSITION_W
  }* pos_e = (pos_t*)&pos;
  while ((pos = s.find(" ")) != std::min(std::string::npos, (size_t)3)) {
    std::string token = s.substr(0, pos);
    switch (*pos_e) {
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
  }
  return v;
}

void CVar::setVec4(glm::vec4 v) {
  setValue(std::format("{} {} {} {}", v.x, v.y, v.z, v.w));
}

static Settings* _singleton = 0;
Settings* Settings::singleton() {
  if (!_singleton) _singleton = new Settings();
  return _singleton;
}

void Settings::parseCommandLine(char* argv[], int argc) {
  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "produce help message")(
      "loadSettings,L", po::value<std::string>(),
      "load settings from custom location")(
      "game,g", po::value<std::string>(),
      "loaded game library path (only works on supported programs like the "
      "launcher)")(
      "hintDs,D",
      "Hint to use dedicated server mode (only works on supported programs)");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    exit(EXIT_FAILURE);
  }

  if (vm.count("loadSettings")) {
    settingsPath = vm["loadSettings"].as<std::string>();
    load();
  }

  if (vm.count("game")) {
    gamePath = vm["game"].as<std::string>();
  }

  hintDs = vm.count("hintDs");

  load();
}

void Settings::load() {
  FILE* sj = fopen(settingsPath.c_str(), "r");
  if (sj) {
    fseek(sj, 0, SEEK_END);
    int sjl = ftell(sj);
    fseek(sj, 0, SEEK_SET);
    char* sjc = (char*)malloc(sjl + 1);
    memset(sjc, 0, sjl + 1);
    fread(sjc, 1, sjl, sj);
    fclose(sj);

    try {
      json j = json::parse(sjc);
      for (auto& item : j["Settings"].items()) {
        settings[item.key()] = item.value();
      }
      for (auto& cvar : j["CVars"].items()) {
        CVar* var = cvars[cvar.key()];
        if (!var) continue;
        if (var->getFlags() & CVARF_SAVE) {
          var->setValue(cvar.value());
        } else {
          throw std::runtime_error("Could not set non CVARF_SAVE cvar");
        }
      }
    } catch (std::exception& e) {
      Log::printf(LOG_ERROR, "Error parsing settings: %s", e.what());
    }
  } else {
    Log::printf(LOG_ERROR, "Couldn't open %s", settingsPath.c_str());
  }
}

void Settings::save() {
  FILE* sj = fopen("settings.json", "w");
  if (sj) {
    json j;
    json _settings = {};
    for (auto item : settings) {
      _settings[item.first] = item.second;
    }
    j["Settings"] = _settings;
    json _cvars = {};
    for (auto cvar : cvars) {
      if (cvar.second->getFlags() & CVARF_SAVE) {
        _cvars[cvar.first] = cvar.second->getValue();
      }
    }
    j["CVars"] = _cvars;
    std::string d = j.dump(1);
    fwrite(d.data(), 1, d.size(), sj);
    fclose(sj);
  }
}

json Settings::getSetting(std::string setting, json unset) {
  auto it = settings.find(setting);
  if (it == settings.end()) {
    settings[setting] = unset;
    return unset;
  } else {
    return settings[setting];
  }
}
}  // namespace rdm
