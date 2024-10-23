#include "settings.hpp"

#include <stdio.h>

#include <boost/program_options.hpp>
#include <iostream>

#include "json.hpp"
#include "logging.hpp"

namespace rdm {
Settings::Settings() {
  settingsPath = "settings.json";
  load();
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
      "logLevel,l", po::value<int>(),
      "0 - debug, 1 - info, 2 - warning, 3 - error, 4 - fatal")
    ("game,g", po::value<std::string>(), "loaded game library path (only works on supported programs like the launcher)");

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

  if (vm.count("logLevel")) {
    settings["LogLevel"] = vm["logLevel"].as<int>();
  }

  if (vm.count("game")) {
    gamePath = vm["game"].as<std::string>();
  }
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
    std::string d = j.dump();
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
