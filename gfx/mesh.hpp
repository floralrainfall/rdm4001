#pragma once
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <memory>
#include <optional>

#include "base_device.hpp"
#include "base_types.hpp"

namespace rdm::gfx {
struct Model {
  std::unique_ptr<const aiScene> scene;
};

class MeshCache {
  Assimp::Importer importer;
  std::map<std::string, std::unique_ptr<Model>> models;

 public:
  MeshCache(BaseDevice* device);

  std::optional<Model> get(const char* path);
};
}  // namespace rdm::gfx
