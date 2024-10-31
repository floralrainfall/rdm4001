#include "wgame.hpp"

#include "filesystem.hpp"
#include "gfx/gui/gui.hpp"
#include "gfx/heightmap.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "map.hpp"
#include "putil/fpscontroller.hpp"
#include "settings.hpp"

namespace ww {
struct WGamePrivate {
  float cameraPitch;
  float cameraYaw;
  putil::FpsController* controller;

  BSPFile* file;
};

using namespace rdm;
WGame::WGame() : Game() {
  Input::singleton()->newAxis("ForwardBackward", SDLK_w, SDLK_s);
  Input::singleton()->newAxis("LeftRight", SDLK_a, SDLK_d);

  game = new WGamePrivate();
}

WGame::~WGame() { delete game; }

void WGame::initializeClient() {
  Input::singleton()->setMouseLocked(true);
  game->file = new BSPFile("dat5/baseq3/maps/malach.bsp");
  game->file->addToPhysicsWorld(world->getPhysicsWorld());
  game->controller = new putil::FpsController(world->getPhysicsWorld());
  world->getPhysicsWorld()->getWorld()->setGravity(btVector3(0, 0, -106.67));

  std::scoped_lock lock(world->worldLock);
  world->stepped.listen([this] {
    gfx::Camera& cam = gfxEngine->getCamera();
    game->controller->updateCamera(cam);
    game->file->updatePosition(cam.getPosition());

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

  gfxEngine->initialized.listen([this] {
    game->file->initGfx(gfxEngine.get());
    gfxEngine->addEntity<MapEntity>(game->file);
  });

  world->getNetworkManager()->connect("127.0.0.1");
}

void WGame::initializeServer() { worldServer->getNetworkManager()->start(); }

void WGame::initialize() {
  if (rdm::Settings::singleton()->getHintDs()) {
    startServer();
  } else {
    startClient();
  }
}
}  // namespace ww
