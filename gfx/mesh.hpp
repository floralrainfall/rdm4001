#pragma once
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <memory>
#include <optional>

#include "base_device.hpp"
#include "base_types.hpp"

namespace rdm::gfx {
class Engine;

struct MeshVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};

struct Mesh {
  std::vector<MeshVertex> vertices;
  std::vector<unsigned int> indices;

  std::unique_ptr<BaseBuffer> vertex;
  std::unique_ptr<BaseBuffer> element;
  std::unique_ptr<BaseArrayPointers> arrayPointers;

  void render(BaseDevice* device);
};

struct Model {
  std::vector<Mesh> meshes;
  aiMesh* mesh;
  const aiScene* scene;
  std::string directory;

  void process(Engine* engine);
  void render(BaseDevice* device);

 private:
  Engine* engine;
  Mesh processMesh(aiMesh* mesh);
  void processNode(Engine* engine, aiNode* node);
};

class MeshCache {
  std::map<std::string, std::unique_ptr<Model>> models;
  Engine* engine;

 public:
  MeshCache(Engine* engine);

  std::optional<Model*> get(const char* path);
};
}  // namespace rdm::gfx
