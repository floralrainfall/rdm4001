#include "wgame.hpp"

#include "filesystem.hpp"
#include "gfx/gui/gui.hpp"
#include "gfx/heightmap.hpp"
#include "input.hpp"
#include "logging.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace ww {
struct WGamePrivate {
  float cameraPitch;
  float cameraYaw;
};

using namespace rdm;
WGame::WGame() : Game() {
  Input::singleton()->newAxis("ForwardBackward", SDLK_w, SDLK_s);
  Input::singleton()->newAxis("LeftRight", SDLK_a, SDLK_d);

  game = new WGamePrivate();
}

WGame::~WGame() { delete game; }

void WGame::initialize() {
  std::scoped_lock lock(world->worldLock);
  world->stepped.listen([this] {
    glm::vec2 mouseDelta = Input::singleton()->getMouseDelta();

    Input::Axis* fbA = Input::singleton()->getAxis("ForwardBackward");
    Input::Axis* lrA = Input::singleton()->getAxis("LeftRight");

    game->cameraPitch += mouseDelta.x * (M_PI / 180.0);
    game->cameraYaw += mouseDelta.y * (M_PI / 180.0);

    glm::quat yawQuat =
        glm::angleAxis(game->cameraYaw, glm::vec3(1.f, 0.f, 0.f));
    glm::quat pitchQuat =
        glm::angleAxis(game->cameraPitch, glm::vec3(0.f, 1.f, 0.f));
    glm::mat3 vm = glm::toMat3(pitchQuat * yawQuat);
    glm::vec3 forward = glm::vec3(0, 0, 1);
    gfx::Camera& cam = gfxEngine->getCamera();
    float speed = 10.0;
    cam.setPosition(
        cam.getPosition() +
        (vm * glm::vec3(lrA->value, 0.0, fbA->value) * speed * (1.f / 60.f)));
    cam.setTarget(cam.getPosition() + vm * forward);

    if (gfxEngine->getGuiManager()) {
      auto& layout = gfxEngine->getGuiManager()->getLayout("Debug");
      char buf[100];
      SchedulerJob* worldJob = world->getScheduler()->getJob("World");
      SchedulerJob* renderJob = world->getScheduler()->getJob("Render");
      snprintf(buf, 100, "World FPS: %0.2f, Render FPS: %0.2f",
               1.0 / worldJob->getStats().totalDeltaTime,
               1.0 / renderJob->getStats().totalDeltaTime);
      layout.getElements()[0].setText(buf);
    }
  });

  {
    std::scoped_lock lock(
        gfxEngine->getContext()->getMutex());  // lock graphics context
    gfxEngine->getContext()->setCurrent();

    try {
      /*
      // initialize graphics here
      std::shared_ptr<gfx::Material> material = gfx::Material::create();
      material->addTechnique(gfx::Technique::create(gfxEngine->getDevice(),
      "dat1/test.vs", "dat1/test.fs"));

      float positions[] = {
        0.f, 0.f,
        1.f, 0.f,
        0.f, 1.0f,
      };
      game->pbuf = gfxEngine->getDevice()->createBuffer();
      game->pbuf->upload(gfx::BaseBuffer::Array, gfx::BaseBuffer::StaticDraw,
      sizeof(positions), positions);

      auto arp = gfxEngine->getDevice()->createArrayPointers();
      arp->addAttrib(gfx::BaseArrayPointers::Attrib(gfx::DtFloat, 0, 2, 0, 0,
      game->pbuf.get())); arp->upload();

      unsigned char index[] = {
        0, 1, 2
      };
      auto buf = gfxEngine->getDevice()->createBuffer();
      buf->upload(gfx::BaseBuffer::Element, gfx::BaseBuffer::StaticDraw,
      sizeof(index), index);
      */

      // ent->setMaterial(gfxEngine->getMaterialCache()->getOrLoad("Sprite").value());

      // gfx::HeightmapEntity* hmap = new gfx::HeightmapEntity(NULL);
      // hmap->generateMesh(gfxEngine.get());
      // gfxEngine->addEntity(std::unique_ptr<gfx::Entity>(hmap));

    } catch (std::exception& e) {
      Log::printf(LOG_ERROR, "Error initializing graphics: %s", e.what());
    }

    gfxEngine->getContext()->unsetCurrent();
  }
}
}  // namespace ww