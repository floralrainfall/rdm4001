#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base_context.hpp"
#include "base_device.hpp"
#include "camera.hpp"
#include "entity.hpp"
#include "gfx/base_types.hpp"
#include "gfx/gui/gui.hpp"
#include "signal.hpp"

namespace rdm {
class World;
}

namespace rdm::gfx {
class TextureCache {
  std::unique_ptr<BaseTexture> invalidTexture;
  BaseDevice* device;

 public:
  struct Info {
    BaseTexture::InternalFormat internalFormat;
    BaseTexture::Format format;
    int width;
    int height;
    int channels;

    unsigned char* data;
  };

  TextureCache(BaseDevice* device);
  std::optional<std::pair<Info, BaseTexture*>> getOrLoad2d(
      const char* path, bool keepData = false);
  std::optional<std::pair<Info, BaseTexture*>> get(const char* path);
  BaseTexture* cacheExistingTexture(const char* path,
                                    std::unique_ptr<BaseTexture>& texture,
                                    Info info);

 private:
  std::map<std::string, std::pair<Info, std::unique_ptr<BaseTexture>>> textures;
};

class Engine {
  friend class RenderJob;

  std::unique_ptr<BaseContext> context;
  std::unique_ptr<BaseDevice> device;
  std::unique_ptr<gui::GuiManager> gui;
  std::vector<std::unique_ptr<Entity>> entities;

  std::unique_ptr<BaseTexture> fullscreenTexture;
  std::unique_ptr<BaseTexture> fullscreenTextureDepth;
  std::unique_ptr<BaseBuffer> fullscreenBuffer;
  std::unique_ptr<BaseArrayPointers> fullScreenArrayPointers;
  std::shared_ptr<Material> fullscreenMaterial;
  std::unique_ptr<BaseFrameBuffer> postProcessFrameBuffer;

  float time;

  Camera cam;

  bool isInitialized;
  void initialize();
  void render();
  void stepped();

  std::unique_ptr<MaterialCache> materialCache;
  std::unique_ptr<TextureCache> textureCache;
  World* world;

 public:
  Engine(World* world, void* hwnd);

  void renderFullscreenQuad(BaseTexture* texture, Material* material = 0);
  void setFullscreenMaterial(const char* name);

  /**
   * @brief This signal will be fired in the Render job thread.
   *
   * It will fire after all Entity's have rendered, and before it unbinds the
   * post-process framebuffer.
   */
  Signal<> renderStepped;
  Signal<> initialized;

  Entity* addEntity(std::unique_ptr<Entity> entity);

  BaseContext* getContext() { return context.get(); }
  BaseDevice* getDevice() { return device.get(); }
  MaterialCache* getMaterialCache() { return materialCache.get(); }
  TextureCache* getTextureCache() { return textureCache.get(); }
  gui::GuiManager* getGuiManager() { return gui.get(); }

  float getTime() { return time; }

  Camera& getCamera() { return cam; }
};
}  // namespace rdm::gfx
