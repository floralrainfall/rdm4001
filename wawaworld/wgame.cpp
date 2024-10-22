#include "wgame.hpp"

#include "filesystem.hpp"
#include "gfx/gui/gui.hpp"
#include "gfx/heightmap.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "map.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace ww {
struct WGamePrivate {
  float cameraPitch;
  float cameraYaw;

  BSPFile* file;
};

using namespace rdm;
WGame::WGame() : Game() {
  Input::singleton()->newAxis("ForwardBackward", SDLK_w, SDLK_s);
  Input::singleton()->newAxis("LeftRight", SDLK_a, SDLK_d);

  game = new WGamePrivate();
}

WGame::~WGame() { delete game; }

void WGame::initialize() {
  Input::singleton()->setMouseLocked(true);
  game->file = new BSPFile("dat5/baseq3/maps/malach.bsp");
  game->file->addToPhysicsWorld(world->getPhysicsWorld());
  
  std::scoped_lock lock(world->worldLock);
  world->stepped.listen([this] {
    glm::vec2 mouseDelta = Input::singleton()->getMouseDelta();

    Input::Axis* fbA = Input::singleton()->getAxis("ForwardBackward");
    Input::Axis* lrA = Input::singleton()->getAxis("LeftRight");

    game->cameraPitch += mouseDelta.x * (M_PI / 180.0);
    game->cameraYaw += mouseDelta.y * (M_PI / 180.0);

    glm::quat yawQuat =
        glm::angleAxis(game->cameraYaw, glm::vec3(-1.f, 0.f, 0.f));
    glm::quat pitchQuat =
        glm::angleAxis(game->cameraPitch, glm::vec3(0.f, 0.f, -1.f));
    glm::mat3 vm = glm::toMat3(pitchQuat * yawQuat);
    glm::vec3 forward = glm::vec3(0, 0, -1);
    gfx::Camera& cam = gfxEngine->getCamera();
    float speed = 100.0;
    glm::vec3 newPosition = (vm * glm::vec3(lrA->value, 0.0, fbA->value) * speed * (1.f / 60.f));
    {
      std::scoped_lock l(world->getPhysicsWorld()->mutex);
      
    }
       
    cam.setPosition(
        cam.getPosition() + newPosition);
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

    game->file->updatePosition(cam.getPosition());
  });

  gfxEngine->initialized.listen([this] {
    game->file->initGfx(gfxEngine.get());
    gfxEngine->addEntity<MapEntity>(game->file);
  });
}
}  // namespace ww
