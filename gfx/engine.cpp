#include "engine.hpp"

#include <stdexcept>

#include "filesystem.hpp"
#include "gl_device.hpp"
#include "logging.hpp"
#include "scheduler.hpp"
#include "settings.hpp"
#include "world.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace rdm::gfx {
TextureCache::TextureCache(BaseDevice* device) {
  this->device = device;
  this->invalidTexture = device->createTexture();
}

std::optional<std::pair<TextureCache::Info, BaseTexture*>>
TextureCache::getOrLoad2d(const char* path, bool keepData) {
  auto it = textures.find(path);
  if (it == textures.end()) {
    TextureCache::Info i;
    common::OptionalData data = common::FileSystem::singleton()->readFile(path);
    if (data) {
      stbi_set_flip_vertically_on_load(true);
      stbi_uc* uc = stbi_load_from_memory(data->data(), data->size(), &i.width,
                                          &i.height, &i.channels, 0);
      if (uc) {
        switch (i.channels) {
          case 3:
            i.format = BaseTexture::RGB;
            i.internalFormat = BaseTexture::RGB8;
            break;
          case 4:
            i.format = BaseTexture::RGBA;
            i.internalFormat = BaseTexture::RGBA8;
            break;
        }

        std::unique_ptr<BaseTexture> tx = device->createTexture();
        tx->upload2d(i.width, i.height, DtUnsignedByte, i.format, uc);
        textures[path] =
            std::pair<TextureCache::Info, std::unique_ptr<BaseTexture>>(
                i, std::move(tx));

        if (keepData) {
          i.data = uc;
        } else {
          i.data = 0;
          stbi_image_free(uc);
        }
	
        return std::pair<TextureCache::Info, BaseTexture*>(i, textures[path].second.get());
      } else {
        throw std::runtime_error("stbi_load_from_memory");
      }
    } else {
      return {};
    }
  } else {
    return std::pair<TextureCache::Info, BaseTexture*>(it->second.first,
                                                       it->second.second.get());
  }
}

std::optional<std::pair<TextureCache::Info, BaseTexture*>> TextureCache::get(
    const char* path) {
  auto it = textures.find(path);
  if (it == textures.end()) {
    return {};
  } else {
    return std::pair<TextureCache::Info, BaseTexture*>(it->second.first,
                                                       it->second.second.get());
  }
}

BaseTexture* TextureCache::cacheExistingTexture(
    const char* path, std::unique_ptr<BaseTexture>& texture, Info info) {
  auto it = textures.find(path);
  if (it == textures.end()) {
    textures[path] =
        std::pair<TextureCache::Info, std::unique_ptr<BaseTexture>>(
            info, std::move(texture));
    return textures[path].second.get();
  } else {
    rdm::Log::printf(
        LOG_ERROR,
        "Attempt to cache existing texture %s even though it is already cached",
        path);
    throw std::runtime_error("Texture already exists");
  }
}

BaseTexture* TextureCache::createCacheTexture(const char* path, Info info) {
  auto it = textures.find(path);
  if (it == textures.end()) {
    textures[path] = std::pair<TextureCache::Info, std::unique_ptr<BaseTexture>>(info, device->createTexture());
  } else {
    throw std::runtime_error("Texture already exists");
  }
}

class RenderJob : public SchedulerJob {
  Engine* engine;

 public:
  RenderJob(Engine* engine) : SchedulerJob("Render"), engine(engine) {}

  virtual double getFrameRate() {
    double renderFr = Settings::singleton()->getSetting("RenderFrameRate", 60.0);
    if(renderFr == 0.0)
      return 0.0;
    return 1.0 / renderFr;
  }

  virtual Result step() {
    BaseDevice* device = engine->device.get();

    try {
      std::scoped_lock lock(engine->context->getMutex());

      engine->context->setCurrent();

      engine->time = getStats().time;

      glm::ivec2 fbSize = engine->context->getBufferSize();
      if (engine->getCamera().getFramebufferSize() !=
          glm::vec2(fbSize.x, fbSize.y)) {
        glm::vec2 fbSizeF = fbSize;
        float s = Settings::singleton()->getSetting("FbScale", 1.0);
        fbSizeF *= s;

        engine->postProcessFrameBuffer->destroyAndCreate();
        engine->fullscreenTexture->destroyAndCreate();
        engine->fullscreenTexture->reserve2d(fbSizeF.x, fbSizeF.y,
                                             BaseTexture::RGBA8);
        engine->fullscreenTextureDepth->destroyAndCreate();
        engine->fullscreenTextureDepth->reserve2d(fbSizeF.x, fbSizeF.y,
                                                  BaseTexture::D24S8);
        engine->postProcessFrameBuffer->setTarget(
            engine->fullscreenTexture.get());
        engine->postProcessFrameBuffer->setTarget(
            engine->fullscreenTextureDepth.get(),
            BaseFrameBuffer::DepthStencil);

        Log::printf(LOG_DEBUG, "Updating size of framebuffer to (%f,%f)",
                    fbSizeF.x, fbSizeF.y);
        if (engine->postProcessFrameBuffer->getStatus() !=
            BaseFrameBuffer::Complete) {
          Log::printf(LOG_ERROR, "BaseFrameBuffer::getStatus() = %i",
                      engine->postProcessFrameBuffer->getStatus());
        }
      }

      void* _ =
          engine->device->bindFramebuffer(engine->postProcessFrameBuffer.get());

      device->viewport(0, 0, fbSize.x, fbSize.y);
      device->clear(0.3, 0.3, 0.3, 0.0);
      device->clearDepth();
      device->setDepthState(BaseDevice::LEqual);
      device->setCullState(BaseDevice::FrontCCW);

      engine->getCamera().updateCamera(glm::vec2(fbSize.x, fbSize.y));

      if (!engine->isInitialized) engine->initialize();

      try {
        engine->render();
      } catch (std::exception& e) {
        Log::printf(LOG_ERROR, "Error in engine->render(), e.what() = %s",
                    e.what());
      }

      engine->device->unbindFramebuffer(_);

      device->setDepthState(BaseDevice::Always);
      device->setCullState(BaseDevice::None);
      engine->renderFullscreenQuad(engine->fullscreenTexture.get());

      engine->context->swapBuffers();

      engine->context->unsetCurrent();
    } catch (std::exception& e) {
      std::scoped_lock lock(engine->context->getMutex());
      Log::printf(LOG_ERROR, "Error in render: %s", e.what());
      engine->context->unsetCurrent();
    }

    return Stepped;
  }
};

Engine::Engine(World* world, void* hwnd) {
  context = std::unique_ptr<BaseContext>(new gl::GLContext(hwnd));
  std::scoped_lock lock(context->getMutex());
  device = std::unique_ptr<BaseDevice>(
      new gl::GLDevice(dynamic_cast<gl::GLContext*>(context.get())));
  device->engine = this;
  textureCache = std::unique_ptr<TextureCache>(new TextureCache(device.get()));
  materialCache =
      std::unique_ptr<MaterialCache>(new MaterialCache(device.get()));

  fullscreenMaterial =
      materialCache->getOrLoad("PostProcess").value_or(nullptr);
  if (fullscreenMaterial) {
    postProcessFrameBuffer = device->createFrameBuffer();
    fullscreenBuffer = device->createBuffer();
    fullscreenBuffer->upload(BaseBuffer::Array, BaseBuffer::StaticDraw,
                             sizeof(float) * 6,
                             (float[]){0.0, 0.0, 2.0, 0.0, 0.0, 2.0});
    fullScreenArrayPointers = device->createArrayPointers();
    fullScreenArrayPointers->addAttrib(BaseArrayPointers::Attrib(
        DataType::DtVec2, 0, 3, 0, 0, fullscreenBuffer.get()));
    fullscreenTexture = device->createTexture();
    fullscreenTextureDepth = device->createTexture();

    fullscreenTextureDepth->reserve2d(800, 600, BaseTexture::D24S8);
    fullscreenTexture->reserve2d(800, 600, BaseTexture::RGBA8);
    postProcessFrameBuffer->setTarget(fullscreenTexture.get());
    postProcessFrameBuffer->setTarget(fullscreenTextureDepth.get(),
                                      BaseFrameBuffer::DepthStencil);

    if (postProcessFrameBuffer->getStatus() != BaseFrameBuffer::Complete) {
      Log::printf(LOG_ERROR, "BaseFrameBuffer::getStatus() = %i",
                  postProcessFrameBuffer->getStatus());
    }
  } else {
    throw std::runtime_error("Could not load PostProcess material!!!");
  }

  renderJob = world->getScheduler()->addJob(new RenderJob(this));
  world->stepped.listen([this] { stepped(); });
  isInitialized = false;
}

void Engine::renderFullscreenQuad(BaseTexture* texture, Material* material) {
  if (material == 0) material = fullscreenMaterial.get();
  BaseProgram* fullscreenProgram = material->prepareDevice(device.get(), 0);
  if (fullscreenProgram) {
    if (texture)
      fullscreenProgram->setParameter(
          "texture0", DtSampler,
          BaseProgram::Parameter{.texture.slot = 0,
                                 .texture.texture = texture});
    fullscreenProgram->bind();
  }
  fullScreenArrayPointers->bind();
  device->draw(fullscreenBuffer.get(), DtFloat, BaseDevice::Triangles, 3);
}

void Engine::setFullscreenMaterial(const char* name) {
  fullscreenMaterial = materialCache->getOrLoad(name).value_or(nullptr);
}

void Engine::stepped() {}

void Engine::render() {
  for (int i = 0; i < entities.size(); i++) {
    Entity* ent = entities[i].get();
    ent->render(device.get());
  }

  renderStepped.fire();
}

void Engine::initialize() {
  gui = std::unique_ptr<gui::GuiManager>(new gui::GuiManager(this));
  isInitialized = true;

  initialized.fire();
}

Entity* Engine::addEntity(std::unique_ptr<Entity> entity) {
  entities.push_back(std::move(entity));
  return (entities.back()).get();
}
}  // namespace rdm::gfx
