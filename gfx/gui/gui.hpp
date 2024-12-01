#pragma once
#include <SDL2/SDL.h>

#include <memory>
#include <string>

#include "gfx/base_types.hpp"
#include "gfx/material.hpp"
#include "script/script.hpp"
#include "signal.hpp"

namespace rdm::gfx {
class Engine;
};

namespace rdm::gfx::gui {
class GuiManager;
struct Component {
  struct Var {
    enum Type { Float, Int, String };
    Type type;
    void* value;
    std::string defaultValue;
  };

  struct Element {
    enum Type { Label, Image };
    Type type;

    std::string value;
    BaseTexture* texture;
    glm::ivec2 textureSize;
  };

  enum Anchor { BottomLeft, BottomRight, TopLeft, TopRight };
  Anchor anchor;

  Signal<> variableChanged;

  std::map<std::string, Var> inVars;
  std::map<std::string, Element> elements;
  std::vector<script::Script> scripts;

  void render(GuiManager* manager, gfx::Engine* engine);
};

class GuiManager {
  friend struct Component;

  gfx::Engine* engine;
  std::shared_ptr<gfx::Material> panel;
  std::shared_ptr<gfx::Material> image;
  std::shared_ptr<gfx::Material> text;

  std::unique_ptr<BaseBuffer> squareArrayBuffer;
  std::unique_ptr<BaseBuffer> squareElementBuffer;
  std::unique_ptr<BaseArrayPointers> squareArrayPointers;

  std::map<std::string, Component> components;
  std::vector<std::unique_ptr<BaseTexture>> textures;

  void parseXml(const char* file);

 public:
  GuiManager(gfx::Engine* engine);

  void render();
};
};  // namespace rdm::gfx::gui
