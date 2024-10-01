#include "gfx/gui/gui.hpp"

#include <SDL2/SDL_ttf.h>

#include "filesystem.hpp"
#include "gfx/engine.hpp"
#include "logging.hpp"

namespace rdm::gfx::gui {
GuiManager::GuiManager(gfx::Engine* engine) {
  gfx::MaterialCache* cache = engine->getMaterialCache();
  panel = cache->getOrLoad("GuiPanel").value();
  text = cache->getOrLoad("GuiText").value();
  image = cache->getOrLoad("GuiImage").value();

  TTF_Init();

  this->engine = engine;
  engine->renderStepped.listen([this] { render(); });

  float squareVtx[] = {0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0};
  squareArrayBuffer = engine->getDevice()->createBuffer();
  squareArrayBuffer->upload(gfx::BaseBuffer::Array, gfx::BaseBuffer::StaticDraw,
                            sizeof(squareVtx), squareVtx);

  unsigned char squareIndex[] = {0, 1, 2, 1, 2, 3};
  squareElementBuffer = engine->getDevice()->createBuffer();
  squareElementBuffer->upload(gfx::BaseBuffer::Element,
                              gfx::BaseBuffer::StaticDraw, sizeof(squareIndex),
                              squareIndex);

  squareArrayPointers = engine->getDevice()->createArrayPointers();
  squareArrayPointers->addAttrib(BaseArrayPointers::Attrib(
      DtFloat, 0, 2, sizeof(float) * 2, 0, squareArrayBuffer.get()));
  squareArrayPointers->upload();

  common::OptionalData ds =
      common::FileSystem::singleton()->readFile("dat3/gui.json");
  if (ds) {
    try {
      std::vector<unsigned char> layoutString = ds.value();
      json j =
          json::parse(std::string(layoutString.begin(), layoutString.end()));
      for (const json& item : j["Layouts"]) {
        Layout l = Layout();
        if (item.contains("Ref")) {
          char buf[255];
          std::string rpath = item["Ref"];
          snprintf(buf, 255, "dat3/%s.json", rpath.c_str());
          common::OptionalData ds =
              common::FileSystem::singleton()->readFile(buf);
          std::vector<unsigned char> refdata = ds.value();
          json ind = json::parse(std::string(refdata.begin(), refdata.end()));
          l.fromJsonData(ind);
        } else {
          l.fromJsonData(item);
        }
        layouts[l.getName()] = l;
      }
    } catch (std::exception& e) {
      Log::printf(LOG_ERROR, "Error creating layouts, what() = %s", e.what());
    }
  }
}

void GuiManager::render() {
  engine->getDevice()->setBlendState(BaseDevice::SrcAlpha,
                                     BaseDevice::OneMinusSrcAlpha);

  BaseProgram* _panel = panel->prepareDevice(engine->getDevice(), 0);
  BaseProgram* _image = image->prepareDevice(engine->getDevice(), 0);
  BaseProgram* _text = text->prepareDevice(engine->getDevice(), 0);
  for (auto& layout : layouts) {
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
              texSurf = TTF_RenderUTF8_Blended(font, element.getText().c_str(),
                                               white);
            if (!texSurf) {
              Log::printf(LOG_DEBUG, "TTF_GetError = %s (Text: %s)",
                          TTF_GetError(), element.getText().c_str());
              throw std::runtime_error("Couldn't render TTF");
            }
            SDL_Surface* texSurfConv =
                SDL_ConvertSurfaceFormat(texSurf, SDL_PIXELFORMAT_ABGR8888, 0);
            SDL_LockSurface(texSurfConv);
            element.uniqueTexture->upload2d(texSurfConv->w, texSurfConv->h,
                                            DtUnsignedByte, BaseTexture::RGBA,
                                            texSurfConv->pixels);
            if (element.override) {
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
                                     .texture.texture = element.uniqueTexture});
          bp->setParameter("scale", DtVec2,
                           BaseProgram::Parameter{.vec2 = element.scale});
          bp->setParameter("offset", DtVec2,
                           BaseProgram::Parameter{.vec2 = element.offset});
          bp->bind();
      }
      squareArrayPointers->bind();
      engine->getDevice()->draw(squareElementBuffer.get(), DtUnsignedByte,
                                BaseDevice::Triangles, 6);
    }
  }
}
}  // namespace rdm::gfx::gui
