#pragma once
#include <SDL2/SDL.h>

#include <memory>
#include <string>

#include "font.hpp"
#include "gfx/base_types.hpp"
#include "gfx/material.hpp"
#include "script/script.hpp"
#include "signal.hpp"

namespace rdm::gfx {
class Engine;
};

namespace rdm::gfx::gui {
class GuiManager;
struct TreeNode {
  std::vector<TreeNode> children;
  std::string elem;

  bool div;
  bool visible;
};

struct Component {
  struct Var {
    enum Type { Float, Int, String };
    Type type;
    void* value;
    std::string defaultValue;
  };

  struct Element {
    enum Type { Label, Image, TextField };
    Type type;

    glm::ivec2 position;
    glm::ivec2 size;

    glm::vec3 color;
    bool link;

    std::string value;
    std::string hover;
    std::string down;
    BaseTexture* texture;
    BaseTexture* textureHover;
    BaseTexture* texturePressed;
    glm::ivec2 textureSize;
    glm::ivec2 minSize;
    bool pressed;

    Signal<> mouseDown;

    Font* font;

    bool dirty;
  };

  enum Anchor { BottomLeft, BottomRight, TopLeft, TopRight };
  enum GrowMode { Horizontal, Vertical };
  Anchor anchor;
  GrowMode grow;

  bool variablesDirty;

  Element* selectedElement;
  GuiManager* manager;

  Signal<> variableChanged;

  std::string parentComponent;
  std::string name;

  std::map<std::string, Var> inVars;

  std::vector<std::string> elementIndex;
  std::map<std::string, Element> elements;
  TreeNode domRoot;

  std::vector<script::Script> scripts;

  bool isVisible();

  void layoutUpdate(TreeNode* root);
  void render(GuiManager* manager, gfx::Engine* engine);
  void scriptUpdate(script::Script* script);
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
  std::unique_ptr<FontCache> fontCache;

  std::map<std::string, Component> components;
  std::vector<std::unique_ptr<BaseTexture>> textures;

  Component* parseXml(const char* file);

 public:
  GuiManager(gfx::Engine* engine);

  gfx::Material* getImageMaterial() { return image.get(); }
  BaseBuffer* getSArrayBuf() { return squareArrayBuffer.get(); }
  BaseBuffer* getSElementBuf() { return squareElementBuffer.get(); }
  BaseArrayPointers* getSArrayPointers() { return squareArrayPointers.get(); }

  // FML
  std::map<std::string, std::unique_ptr<BaseTexture>> namedTextures;
  FontCache* getFontCache() { return fontCache.get(); }

  std::optional<Component*> getComponentByName(std::string name) {
    if (components.find(name) != components.end())
      return &components[name];
    else
      return {};
  }

  void render();
  Engine* getEngine() { return engine; }
};
};  // namespace rdm::gfx::gui
