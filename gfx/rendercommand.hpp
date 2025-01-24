#pragma once
#include <algorithm>
#include <optional>
#include <vector>

#include "gfx/base_device.hpp"
#include "gfx/base_types.hpp"
namespace rdm::gfx {
class Engine;

struct DirtyFields {
  bool program;
  bool pointers;
};

#define NR_MAX_TEXTURES 4

class RenderCommand {
  gfx::BaseProgram* program;
  gfx::BaseArrayPointers* pointers;
  gfx::BaseBuffer* elements;
  gfx::BaseDevice::DrawType type;
  gfx::BaseTexture* texture[NR_MAX_TEXTURES];
  std::optional<glm::mat4> model;
  size_t count;
  void* first;

 public:
  RenderCommand(gfx::BaseDevice::DrawType type, gfx::BaseBuffer* elements,
                size_t count, gfx::BaseArrayPointers* pointers = 0,
                gfx::BaseProgram* program = 0, void* first = 0);

  void setTexture(int id, gfx::BaseTexture* texture) {
    this->texture[id] = texture;
  }
  void setModel(std::optional<glm::mat4> model) { this->model = model; }
  gfx::BaseTexture* getTexture(int id) const { return texture[id]; }
  std::optional<glm::mat4> getModel() const { return model; };

  DirtyFields render(gfx::Engine* engine);
};

struct RenderListSettings {
  BaseDevice::CullState cull;
  BaseDevice::DepthStencilState state;

  RenderListSettings() {
    cull = BaseDevice::FrontCCW;
    state = BaseDevice::LEqual;
  }
};

class RenderList {
  gfx::BaseProgram* program;
  gfx::BaseArrayPointers* pointers;

  std::vector<RenderCommand> commands;
  RenderListSettings settings;

 public:
  RenderList(gfx::BaseProgram* program, gfx::BaseArrayPointers* pointers,
             RenderListSettings settings = RenderListSettings());

  void clear() { commands.clear(); }
  void add(RenderCommand& command);
  void render(gfx::Engine* engine);

  template <typename T>
  void sort(T fun) {
    std::sort(commands.begin(), commands.end(), fun);
  }
};
};  // namespace rdm::gfx
