#pragma once
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <deque>
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

struct MeshVertexSkinned : public MeshVertex {
  glm::ivec4 boneIds;
  glm::vec4 boneWeights;
};

struct BoneInfo {
  int id;
  glm::mat4 offset;
};

struct Mesh {
  bool skinned;

  std::map<std::string, BoneInfo> bones;
  std::vector<unsigned int> indices;

  std::unique_ptr<BaseBuffer> vertex;
  std::unique_ptr<BaseBuffer> element;
  std::unique_ptr<BaseArrayPointers> arrayPointers;

  void render(BaseDevice* device);
};

struct Model {
  int boneCounter;
  std::vector<Mesh> meshes;
  std::vector<std::string> textures;
  aiMesh* mesh;
  const aiScene* scene;
  std::string directory;
  bool precache;

  void process(Engine* engine);
  void render(BaseDevice* device);

  ~Model();

 private:
  Engine* engine;
  Mesh processMesh(aiMesh* mesh);
  void processNode(Engine* engine, aiNode* node);
};

class Primitive {
  std::unique_ptr<BaseBuffer> vertex;
  std::unique_ptr<BaseBuffer> element;
  std::unique_ptr<BaseArrayPointers> arrayPointers;
  size_t indicesCount;

 public:
  enum Type {
    PlaneZ,

    Count,
  };

  Primitive(Type type, Engine* engine);
  void render(BaseDevice* device);

 private:
  Type t;
};

class MeshCache {
  std::map<std::string, std::unique_ptr<Model>> models;
  std::vector<Primitive> primitives;

  Engine* engine;

 public:
  MeshCache(Engine* engine);

  std::optional<Model*> get(const char* path);
  Primitive* get(Primitive::Type type);
  void del(const char* path);
};
}  // namespace rdm::gfx
