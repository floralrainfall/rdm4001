#include "material.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include "logging.hpp"
#include "filesystem.hpp"
#include "json.hpp"
using json = nlohmann::json;

namespace rdm::gfx {
  static ShaderCache* _singleton;
  ShaderCache* ShaderCache::singleton() {
    if(!_singleton) {
      _singleton = new ShaderCache();
    }
    return _singleton;
  }

  ShaderFile ShaderCache::getCachedOrFile(const char* path) {
    auto it = cache.find(path);
    ShaderFile f;
    f.name = path;
    if(it != cache.end()) {
      f.code = cache[path];
    } else {
      common::OptionalData data = common::FileSystem::singleton()->readFile(path);
      if(data) {
        std::string code = std::string(data->begin(), data->end());
        cache[path] = code;
        f.code = code;
      } else {
        Log::printf(LOG_ERROR, "Could not load shader file %s", path);
        throw std::runtime_error("Could not load shader file");
      }
    }
    return f;
  }

  Technique::Technique(BaseDevice* device, std::string techniqueVs, std::string techniqueFs) {
    program = device->createProgram();
    program->addShader(ShaderCache::singleton()->getCachedOrFile(techniqueVs.c_str()), BaseProgram::Vertex);
    program->addShader(ShaderCache::singleton()->getCachedOrFile(techniqueFs.c_str()), BaseProgram::Fragment);
    program->link();
  }

  void Technique::bindProgram() {
    program->bind();
  }

  std::shared_ptr<Technique> Technique::create(BaseDevice* device, std::string techniqueVs, std::string techniqueFs) {
    return std::shared_ptr<Technique>(new Technique(device, techniqueVs, techniqueFs));
  }

  Material::Material() {

  }

  void Material::addTechnique(std::shared_ptr<Technique> qu) {
    techniques.push_back(qu);
  }

  void Material::prepareDevice(BaseDevice* device, int techniqueId) {
    if(techniques.size() < techniqueId)
      return;
    techniques[techniqueId]->bindProgram();
  }
  
  std::shared_ptr<Material> Material::create() {
    return std::shared_ptr<Material>(new Material());
  }

  MaterialCache::MaterialCache(BaseDevice* device) {
    this->device = device;
    std::vector<unsigned char> materialJsonString = common::FileSystem::singleton()->readFile("dat1/materials.json").value();
    materialData = std::string(materialJsonString.begin(), materialJsonString.end());
  }

  std::optional<std::shared_ptr<Material>> MaterialCache::getOrLoad(const char* materialName) {
    auto it = cache.find(materialName);
    if(it != cache.end()) {
      return cache[materialName];
    } else {
      std::shared_ptr<Material> material = Material::create();
      try {
        json data = json::parse(materialData);
        json materialInfo = data["Materials"][materialName];
        if(materialInfo.is_null()) {
          Log::printf(LOG_ERROR, "Could not find material %s", materialName);
          return {};
        }
        json techniques = materialInfo["Techniques"];
        int techniqueId = 0;
        for(const json& item : techniques) {
          std::string programName = item["ProgramName"];
          json program = data["Programs"][programName];
          try {
            if(program.is_null()) {
              Log::printf(LOG_ERROR, "Could not find program for technique %i", techniqueId);
              continue;
            }
            std::string vsName = program["VSName"];
            std::string fsName = program["FSName"];
            material->addTechnique(Technique::create(device, vsName, fsName));
          } catch(std::runtime_error& e) {
            Log::printf(LOG_ERROR, "Couldn't compile Technique what() = %s", e.what());
            continue;
          }
          techniqueId++;
        }
        Log::printf(LOG_DEBUG, "Cached new material %s", materialName);
        cache[materialName] = material;
        return material;
      } catch(std::runtime_error& e) {
        Log::printf(LOG_ERROR, "Fucked up MaterialCache::getOrLoad sorry  for making it json what() = ", e.what());
        return {};
      }
    }
  }
}