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
#include "gfx/mesh.hpp"
#include "scheduler.hpp"
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
  BaseTexture* createCacheTexture(const char* path, Info info);

 private:
  std::map<std::string, std::pair<Info, std::unique_ptr<BaseTexture>>> textures;
};

class Engine {
  friend class RenderJob;
  SchedulerJob* renderJob;

  std::unique_ptr<BaseContext> context;
  std::unique_ptr<BaseDevice> device;
  std::unique_ptr<gui::GuiManager> gui;
  std::vector<std::unique_ptr<Entity>> entities;

  std::unique_ptr<BaseTexture> fullscreenTexture;
  std::unique_ptr<BaseTexture> fullscreenTextureBloom;
  std::unique_ptr<BaseTexture> fullscreenTextureDepth;
  std::unique_ptr<BaseBuffer> fullscreenBuffer;
  std::unique_ptr<BaseArrayPointers> fullScreenArrayPointers;
  std::shared_ptr<Material> fullscreenMaterial;
  std::unique_ptr<BaseFrameBuffer> postProcessFrameBuffer;
  int fullscreenSamples;

  float time;

  Camera cam;

  bool isInitialized;
  void initialize();
  void render();
  void stepped();

  std::unique_ptr<MaterialCache> materialCache;
  std::unique_ptr<TextureCache> textureCache;
  std::unique_ptr<MeshCache> meshCache;
  World* world;

 public:
  Engine(World* world, void* hwnd);

  void renderFullscreenQuad(
      BaseTexture* texture, Material* material = 0,
      std::function<void(BaseProgram*)> setParameters = [](BaseProgram*) {});
  void setFullscreenMaterial(const char* name);

  /**
   * @brief This signal will be fired in the Render job thread.
   *
   * It will fire after all Entity's have rendered, and before it unbinds the
   * post-process framebuffer.
   */
  Signal<> renderStepped;
  Signal<> initialized;

  /**
   * @brief Use Engine::addEntity<T>();
   */
  Entity* addEntity(std::unique_ptr<Entity> entity);
  void deleteEntity(Entity* entity);
  SchedulerJob* getRenderJob() { return renderJob; }

  template <typename T, class... Types>
  T* addEntity(Types... args) {
    static_assert(std::is_base_of<Entity, T>::value,
                  "T must derive from Entity");
    T* t = (T*)addEntity(std::unique_ptr<Entity>(new T(args...)));
    t->initialize();
    return t;
  }

  BaseContext* getContext() { return context.get(); }
  BaseDevice* getDevice() { return device.get(); }
  MaterialCache* getMaterialCache() { return materialCache.get(); }
  TextureCache* getTextureCache() { return textureCache.get(); }
  MeshCache* getMeshCache() { return meshCache.get(); }
  gui::GuiManager* getGuiManager() { return gui.get(); }

  float getTime() { return time; }

  Camera& getCamera() { return cam; }
};
}  // namespace rdm::gfx
