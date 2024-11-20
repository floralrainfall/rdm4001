#include "mesh.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "engine.hpp"
#include "filesystem.hpp"
#include "gfx/base_types.hpp"
#include "logging.hpp"

namespace rdm::gfx {
void Model::process(Engine* engine) {
  this->engine = engine;
  processNode(engine, scene->mRootNode);
}

void Model::processNode(Engine* engine, aiNode* node) {
  for (int i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(processMesh(mesh));
  }
  for (int i = 0; i < node->mNumChildren; i++) {
    processNode(engine, node->mChildren[i]);
  }
}

void Mesh::render(BaseDevice* device) {
  arrayPointers->bind();
  device->draw(element.get(), DtUnsignedInt, BaseDevice::Triangles,
               indices.size());
}

void Model::render(BaseDevice* device) {
  for (auto& mesh : meshes) mesh.render(device);
}

Mesh Model::processMesh(aiMesh* mesh) {
  Mesh m;
  for (int i = 0; i < mesh->mNumVertices; i++) {
    MeshVertex vertex;
    vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                mesh->mVertices[i].z);
    vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                              mesh->mNormals[i].z);
    // vertex.uv =
    //     glm::vec2(mesh->mTextureCoords[i][0].x,
    //     mesh->mTextureCoords[i][0].y);
    m.vertices.push_back(vertex);
  }
  for (int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (int i = 0; i < face.mNumIndices; i++) {
      m.indices.push_back(face.mIndices[i]);
    }
  }
  if (mesh->mMaterialIndex >= 0) {
  }
  m.vertex = engine->getDevice()->createBuffer();
  m.element = engine->getDevice()->createBuffer();

  m.vertex->upload(BaseBuffer::Array, BaseBuffer::StaticDraw,
                   m.vertices.size() * sizeof(MeshVertex), m.vertices.data());
  m.element->upload(BaseBuffer::Element, BaseBuffer::StaticDraw,
                    m.indices.size() * sizeof(unsigned int), m.indices.data());

  m.arrayPointers = engine->getDevice()->createArrayPointers();
  m.arrayPointers->addAttrib(BaseArrayPointers::Attrib(
      DtFloat, 0, 3, sizeof(MeshVertex), (void*)offsetof(MeshVertex, position),
      m.vertex.get()));
  m.arrayPointers->addAttrib(BaseArrayPointers::Attrib(
      DtFloat, 1, 3, sizeof(MeshVertex), (void*)offsetof(MeshVertex, normal),
      m.vertex.get()));
  m.arrayPointers->addAttrib(BaseArrayPointers::Attrib(
      DtFloat, 2, 2, sizeof(MeshVertex), (void*)offsetof(MeshVertex, uv),
      m.vertex.get()));
  m.arrayPointers->upload();

  Log::printf(LOG_DEBUG, "Created mesh with %i vertices, %i indices",
              m.vertices.size(), m.indices.size());

  return m;
}

Primitive::Primitive(Type type, Engine* engine) {
  std::vector<MeshVertex> vertices;
  std::vector<unsigned char> indices;
  switch (type) {
    case PlaneZ:
      vertices.push_back(MeshVertex{.position = glm::vec3(0.0, 0.0, 0.0),
                                    .normal = glm::vec3(0.0, 0.0, 1.0),
                                    .uv = glm::vec2(0.0, 0.0)});
      vertices.push_back(MeshVertex{.position = glm::vec3(1.0, 0.0, 0.0),
                                    .normal = glm::vec3(0.0, 0.0, 1.0),
                                    .uv = glm::vec2(1.0, 0.0)});
      vertices.push_back(MeshVertex{.position = glm::vec3(0.0, 1.0, 0.0),
                                    .normal = glm::vec3(0.0, 0.0, 1.0),
                                    .uv = glm::vec2(0.0, 1.0)});
      vertices.push_back(MeshVertex{.position = glm::vec3(1.0, 1.0, 0.0),
                                    .normal = glm::vec3(0.0, 0.0, 1.0),
                                    .uv = glm::vec2(1.0, 1.0)});

      indices.push_back(0);
      indices.push_back(1);
      indices.push_back(2);
      indices.push_back(2);
      indices.push_back(1);
      indices.push_back(3);
      break;
    default:
      Log::printf(LOG_ERROR, "Unknown type %i", type);
      break;
  }
  vertex = engine->getDevice()->createBuffer();
  element = engine->getDevice()->createBuffer();

  vertex->upload(BaseBuffer::Array, BaseBuffer::StaticDraw,
                 vertices.size() * sizeof(MeshVertex), vertices.data());
  element->upload(BaseBuffer::Element, BaseBuffer::StaticDraw,
                  indices.size() * sizeof(unsigned char), indices.data());

  arrayPointers = engine->getDevice()->createArrayPointers();
  arrayPointers->addAttrib(BaseArrayPointers::Attrib(
      DtFloat, 0, 3, sizeof(MeshVertex), (void*)offsetof(MeshVertex, position),
      vertex.get()));
  arrayPointers->addAttrib(BaseArrayPointers::Attrib(
      DtFloat, 1, 3, sizeof(MeshVertex), (void*)offsetof(MeshVertex, normal),
      vertex.get()));
  arrayPointers->addAttrib(
      BaseArrayPointers::Attrib(DtFloat, 2, 2, sizeof(MeshVertex),
                                (void*)offsetof(MeshVertex, uv), vertex.get()));
  arrayPointers->upload();

  indicesCount = indices.size();
}

void Primitive::render(BaseDevice* device) {
  arrayPointers->bind();
  device->draw(element.get(), DtUnsignedByte, BaseDevice::Triangles,
               indicesCount);
}

MeshCache::MeshCache(Engine* engine) {
  this->engine = engine;
  for (int i = 0; i < Primitive::Count; i++) {
    primitives.push_back(Primitive((Primitive::Type)i, engine));
  }
}

Primitive* MeshCache::get(Primitive::Type type) { return &primitives[type]; }

std::optional<Model*> MeshCache::get(const char* path) {
  auto it = models.find(path);
  if (it != models.end()) {
    return models[path].get();
  } else {
    Assimp::Importer importer;
    common::OptionalData data = common::FileSystem::singleton()->readFile(path);
    if (data) {
      Model* model = new Model();
      model->directory = std::string(path).find_last_of('/');
      model->scene = importer.ReadFileFromMemory(
          data.value().data(), data.value().size(),
          aiProcess_Triangulate | aiProcess_FlipUVs);
      if (!model->scene || model->scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
          !model->scene->mRootNode) {
        Log::printf(LOG_ERROR, "Error importing from assimp: %s",
                    importer.GetErrorString());
        return {};
      }

      model->process(engine);
      models[path].reset(model);
      return models[path].get();
    } else {
      Log::printf(LOG_WARN, "Could not open model %s", path);
      return {};
    }
  }
}
}  // namespace rdm::gfx
