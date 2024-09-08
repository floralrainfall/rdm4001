#pragma once
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <optional>
#include "base_types.hpp"
#include "base_device.hpp"

namespace rdm::gfx {
  // TODO: this kinda sucks cause it doesnt store shader objects but shader code
  class ShaderCache {
    std::map<std::string, std::string> cache;
  public:
    static ShaderCache* singleton();

    ShaderFile getCachedOrFile(const char* path);
  };

  class Technique {
    std::unique_ptr<BaseProgram> program;
    Technique(BaseDevice* device, std::string techniqueVs, std::string techniqueFs);
  public:

    static std::shared_ptr<Technique> create(BaseDevice* device, std::string techniqueVs, std::string techniqueFs);

    void bindProgram();
  };

  class Material {
    std::vector<std::shared_ptr<Technique>> techniques;
    Material();
  public:

    static std::shared_ptr<Material> create();

    void addTechnique(std::shared_ptr<Technique> qu);

    int numTechniques() { return techniques.size(); }
    void prepareDevice(BaseDevice* device, int techniqueId);
  };
  
  class MaterialCache {
    std::map<std::string, std::shared_ptr<Material>> cache;
    std::string materialData;
    BaseDevice* device;
    
  public:
    MaterialCache(BaseDevice* device);

    std::optional<std::shared_ptr<Material>> getOrLoad(const char* materialName);
  };
}