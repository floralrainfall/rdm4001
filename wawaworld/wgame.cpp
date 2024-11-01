#include "wgame.hpp"

#include "filesystem.hpp"
#include "gfx/gui/gui.hpp"
#include "gfx/heightmap.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "map.hpp"
#include "network/entity.hpp"
#include "putil/fpscontroller.hpp"
#include "settings.hpp"
#include "worldspawn.hpp"

namespace ww {
struct WGamePrivate {
  float cameraPitch;
  float cameraYaw;
  putil::FpsController* controller;

  Worldspawn* worldspawn;
};

using namespace rdm;
WGame::WGame() : Game() {
  Input::singleton()->newAxis("ForwardBackward", SDLK_w, SDLK_s);
  Input::singleton()->newAxis("LeftRight", SDLK_a, SDLK_d);

  game = new WGamePrivate();
}

WGame::~WGame() { delete game; }

void WGame::addEntityConstructors(network::NetworkManager* manager) {
  manager->registerConstructor(network::EntityConstructor<Worldspawn>,
                               "Worldspawn");
}

void WGame::initializeClient() {
  addEntityConstructors(getWorld()->getNetworkManager());
  game->worldspawn = 0;

  Input::singleton()->setMouseLocked(true);
  game->controller = new putil::FpsController(world->getPhysicsWorld());
  world->getPhysicsWorld()->getWorld()->setGravity(btVector3(0, 0, -106.67));

  std::scoped_lock lock(world->worldLock);
  world->stepped.listen([this] {
    gfx::Camera& cam = gfxEngine->getCamera();
    game->controller->updateCamera(cam);
    if (!game->worldspawn)
      game->worldspawn =
          (Worldspawn*)world->getNetworkManager()->findEntityByType(
              "Worldspawn");
    // game->file->updatePosition(cam.getPosition());

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
    // game->file->initGfx(gfxEngine.get());
    // gfxEngine->addEntity<MapEntity>(game->file);
  });

  gfxEngine->renderStepped.listen([this] {
    if (game->worldspawn) {
      if (game->worldspawn->isPendingAddToGfx()) {
        game->worldspawn->addToEngine(gfxEngine.get());
      }
      game->worldspawn->getFile()->updatePosition(
          gfxEngine->getCamera().getPosition());
    }
  });

  world->getNetworkManager()->connect("127.0.0.1");
}

void WGame::initializeServer() {
  addEntityConstructors(worldServer->getNetworkManager());
  worldServer->getNetworkManager()->start();
  worldServer->getPhysicsWorld()->getWorld()->setGravity(
      btVector3(0, 0, -106.67));

  Worldspawn* wspawn =
      (Worldspawn*)worldServer->getNetworkManager()->instantiate("Worldspawn");
  wspawn->loadFile("dat5/baseq3/maps/malach.bsp");
}

void WGame::initialize() {
  if (rdm::Settings::singleton()->getHintDs()) {
    startServer();
  } else {
    startClient();
  }
}
}  // namespace ww
