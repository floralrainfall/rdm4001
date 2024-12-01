#include "gfx/gui/gui.hpp"

#include <SDL2/SDL_ttf.h>

#include <format>
#include <numeric>

#include "filesystem.hpp"
#include "gfx/base_types.hpp"
#include "gfx/engine.hpp"
#include "logging.hpp"
#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"

namespace rdm::gfx::gui {
GuiManager::GuiManager(gfx::Engine* engine) {
  gfx::MaterialCache* cache = engine->getMaterialCache();
  panel = cache->getOrLoad("GuiPanel").value();
  text = cache->getOrLoad("GuiText").value();
  image = cache->getOrLoad("GuiImage").value();

  this->engine = engine;

  float squareVtx[] = {0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0};
  squareArrayBuffer = engine->getDevice()->createBuffer();
  squareArrayBuffer->upload(gfx::BaseBuffer::Array, gfx::BaseBuffer::StaticDraw,
                            sizeof(squareVtx), squareVtx);

  unsigned char squareIndex[] = {0, 1, 2, 2, 1, 3};
  squareElementBuffer = engine->getDevice()->createBuffer();
  squareElementBuffer->upload(gfx::BaseBuffer::Element,
                              gfx::BaseBuffer::StaticDraw, sizeof(squareIndex),
                              squareIndex);

  squareArrayPointers = engine->getDevice()->createArrayPointers();
  squareArrayPointers->addAttrib(BaseArrayPointers::Attrib(
      DtFloat, 0, 2, sizeof(float) * 2, 0, squareArrayBuffer.get()));
  squareArrayPointers->upload();

  parseXml("dat3/root.xml");
  for (auto& component : components) {
    component.second.variableChanged.fire();
  }
}

void Component::render(GuiManager* manager, gfx::Engine* engine) {
  for (auto& element : elements) {
    switch (element.second.type) {
      case Element::Image: {
        gfx::BaseProgram* bp =
            manager->image->prepareDevice(engine->getDevice(), 0);
        bp->setParameter(
            "texture0", DtSampler,
            BaseProgram::Parameter{.texture.slot = 0,
                                   .texture.texture = element.second.texture});
        bp->setParameter(
            "scale", DtVec2,
            BaseProgram::Parameter{.vec2 = element.second.textureSize});
        bp->setParameter("offset", DtVec2,
                         BaseProgram::Parameter{.vec2 = glm::ivec2(0, 0)});
        bp->bind();
        manager->squareArrayPointers->bind();
        engine->getDevice()->draw(manager->squareElementBuffer.get(),
                                  DtUnsignedByte, BaseDevice::Triangles, 6);
      } break;
      default:
        break;
    }
  }
}

void GuiManager::parseXml(const char* file) {
  common::OptionalData d = common::FileSystem::singleton()->readFile(file);
  if (d) {
    try {
      Component component;

      rapidxml::xml_document<> doc;
      doc.parse<0>((char*)d->data());
      rapidxml::xml_node<>* node = doc.first_node();

      std::map<std::string, Component::Anchor> anchors;
      anchors["BottomLeft"] = Component::BottomLeft;
      anchors["BottomRight"] = Component::BottomRight;
      anchors["TopLeft"] = Component::TopLeft;
      anchors["TopRight"] = Component::TopRight;
      rapidxml::xml_attribute<>* anchor = node->first_attribute("anchor");
      if (anchor) {
        component.anchor = anchors[anchor->value()];
      } else {
        component.anchor = Component::TopLeft;
      }

      auto it = components.find(node->first_attribute("name")->value());
      if (it != components.end()) {
        return;  // already exists
      }
      for (rapidxml::xml_node<>* child = node->first_node(); child;
           child = child->next_sibling()) {
        std::string name = child->name();
        if (name == "component") {
          std::string src = child->first_attribute("src")->value();
          parseXml(src.c_str());
        } else if (name == "script") {
          script::Script s;
#ifndef NDEBUG
          Log::printf(LOG_DEBUG, "Loaded script\n%s", child->value());
#endif

          s.load(child->value());
          component.scripts.push_back(s);
        } else if (name == "in_vars") {
          for (rapidxml::xml_node<>* _child = child->first_node(); _child;
               _child = _child->next_sibling()) {
            std::string name = _child->first_attribute("name")->value();
            std::string type = _child->first_attribute("type")->value();
            std::string defaultVar = _child->value();

            Component::Var v;

            std::map<std::string, Component::Var::Type> types;
            types["float"] = Component::Var::Float;
            types["int"] = Component::Var::Int;
            types["string"] = Component::Var::String;

            v.type = types[type];
            v.defaultValue = defaultVar;
            switch (v.type) {
              case Component::Var::Float:
                v.value = new float;
                *(float*)v.value = std::atof(v.defaultValue.c_str());
                break;
              case Component::Var::Int:
                v.value = new int;
                *(int*)v.value = std::atoi(v.defaultValue.c_str());
                break;
              case Component::Var::String:
                v.value = new std::string;
                *(std::string*)v.value = v.defaultValue;
                break;
            }
            component.inVars[name] = v;
          }
        } else if (name == "label") {
          Component::Element em;
          em.type = Component::Element::Label;
          em.value = child->value();

          rapidxml::xml_attribute<>* id = child->first_attribute("id");
          if (id) {
            component.elements[id->value()] = em;
          } else {
            std::string newid =
                std::format("label-{}", component.elements.size());
            component.elements[newid] = em;
          }
        } else if (name == "image") {
          Component::Element em;
          em.type = Component::Element::Image;
          auto t = engine->getTextureCache()
                       ->getOrLoad2d(child->first_attribute("src")->value())
                       .value();
          em.textureSize = glm::ivec2(t.first.width, t.first.height);
          em.texture = t.second;

          rapidxml::xml_attribute<>* id = child->first_attribute("id");
          if (id) {
            component.elements[id->value()] = em;
          } else {
            std::string newid =
                std::format("image-{}", component.elements.size());
            component.elements[newid] = em;
          }
        } else {
          Log::printf(LOG_WARN, "Unknown element %s", name.c_str());
        }
      }

      std::string name = node->first_attribute("name")->value();
      component.variableChanged.listen([component, name] {
        Log::printf(LOG_DEBUG, "Component %s changed, updating", name.c_str());
        for (auto script : component.scripts) {
          script.run();
        }
      });

      components[name] = component;
      Log::printf(LOG_DEBUG, "Loaded XML %s", file);
    } catch (rapidxml::parse_error& e) {
      Log::printf(LOG_ERROR, "XML parse error: %s (%s)", file, e.what());
    } catch (std::exception& e) {
      Log::printf(LOG_ERROR, "Error loading XML: %s (%s)", file, e.what());
    }
  } else {
    Log::printf(LOG_ERROR, "Error loading XML: %s", file);
  }
}

void GuiManager::render() {
  engine->getDevice()->setBlendState(BaseDevice::SrcAlpha,
                                     BaseDevice::OneMinusSrcAlpha);
  BaseFrameBuffer::AttachmentPoint attachments[] = {
      BaseFrameBuffer::Color0,
      BaseFrameBuffer::Color1,
  };
  engine->getDevice()->targetAttachments(attachments, 1);

  BaseProgram* _panel = panel->prepareDevice(engine->getDevice(), 0);
  BaseProgram* _image = image->prepareDevice(engine->getDevice(), 0);
  BaseProgram* _text = text->prepareDevice(engine->getDevice(), 0);

  for (auto& component : components) {
    component.second.render(this, engine);
  }

  /*for (auto& layout : layouts) {
    if (!layout.second.overrideSize &&
        layout.second.size != engine->getCamera().getFramebufferSize()) {
      layout.second.update(engine->getCamera().getFramebufferSize());
    }

    for (auto& element : layout.second.getElements()) {
      BaseProgram* bp = _panel;
      switch (element.type) {
        case Element::Image:
          bp = _image;
          bp->setParameter("texture0", DtSampler,
                           BaseProgram::Parameter{
                               .texture.slot = 0,
                               .texture.texture =
                                   engine->getTextureCache()
                                       ->getOrLoad2d(element.getText().c_str())
                                       .value()
                                       .second});
        case Element::Panel:
          bp->setParameter("scale", DtVec2,
                           BaseProgram::Parameter{.vec2 = element.scale});
          bp->setParameter("offset", DtVec2,
                           BaseProgram::Parameter{.vec2 = element.offset});
          bp->bind();
          break;
        case Element::Text:
          if (!element.uniqueTexture) {
            std::unique_ptr<BaseTexture> bx =
                engine->getDevice()->createTexture();
            element.uniqueTexture = bx.get();
            textures.push_back(std::move(bx));
          }

          if (element.isDirty()) {
            std::string fontName =
                element.font + std::to_string(element.fontSize);
            auto it = fonts.find(fontName);
            if (it == fonts.end()) {
              common::OptionalData ds =
                  common::FileSystem::singleton()->readFile(
                      element.font.c_str());
              if (ds) {
                void* dcpy = malloc(ds->size());
                memcpy(dcpy, ds->data(), ds->size());
                SDL_RWops* fontMem = SDL_RWFromConstMem(dcpy, ds->size());
                TTF_Font* fontOut =
                    TTF_OpenFontRW(fontMem, 0, element.fontSize);
                if (!fontOut) {
                  throw std::runtime_error("Error creating font");
                }
                fonts[fontName] = fontOut;
              } else {
                throw std::runtime_error("I cant find the font");
              }
            }

            TTF_Font* font = (TTF_Font*)fonts[fontName];
            SDL_Color white;
            white.r = 255;
            white.g = 255;
            white.b = 255;
            white.a = 255;
            SDL_Surface* texSurf = 0;
            if (element.wrapped)
              texSurf = TTF_RenderUTF8_Blended_Wrapped(
                  font, element.getText().c_str(), white, element.maxScale.x);
            else
              texSurf = TTF_RenderUTF8_Blended(font,
    element.getText().c_str(), white); if (!texSurf) { Log::printf(LOG_DEBUG,
    "TTF_GetError = %s (Text: %s)", TTF_GetError(),
    element.getText().c_str()); throw std::runtime_error("Couldn't render
    TTF");
            }
            SDL_Surface* texSurfConv =
                SDL_ConvertSurfaceFormat(texSurf, SDL_PIXELFORMAT_ABGR8888,
    0); SDL_LockSurface(texSurfConv);
            element.uniqueTexture->upload2d(texSurfConv->w, texSurfConv->h,
                                            DtUnsignedByte, BaseTexture::RGBA,
                                            texSurfConv->pixels);
            if (element.override) {
              element.maxScale = glm::vec2(texSurfConv->w, texSurfConv->h);
              layout.second.update(
                  layout.second.size);  // requires layout update to take max
                                        // scale into account
            }
            SDL_UnlockSurface(texSurfConv);
            SDL_FreeSurface(texSurfConv);
            SDL_FreeSurface(texSurf);

            element.clearDirty();
          }

          bp = _text;
          bp->setParameter(
              "texture0", DtSampler,
              BaseProgram::Parameter{.texture.slot = 0,
                                     .texture.texture =
    element.uniqueTexture}); bp->setParameter("scale", DtVec2,
                           BaseProgram::Parameter{.vec2 = element.scale});
          bp->setParameter("offset", DtVec2,
                           BaseProgram::Parameter{.vec2 = element.offset});
          bp->bind();
      }
      squareArrayPointers->bind();
      engine->getDevice()->draw(squareElementBuffer.get(), DtUnsignedByte,
                                BaseDevice::Triangles, 6);
    }
    }*/

  engine->getDevice()->targetAttachments(attachments, 2);
}
}  // namespace rdm::gfx::gui
