#include "wgame.hpp"

#include "filesystem.hpp"
#include "gfx/gui/gui.hpp"
#include "gfx/heightmap.hpp"
#include "gfx/imgui/imgui.h"
#include "input.hpp"
#include "logging.hpp"
#include "map.hpp"
#include "network/entity.hpp"
#include "network/network.hpp"
#include "putil/fpscontroller.hpp"
#include "settings.hpp"
#include "worldspawn.hpp"
#include "wplayer.hpp"

namespace ww {
enum UIState {
  MainMenu,
  ConnectPanel,
};

struct WGamePrivate {
  float cameraPitch;
  float cameraYaw;
  putil::FpsController* controller;

  Worldspawn* worldspawn;
  UIState state;
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
  manager->registerConstructor(network::EntityConstructor<WPlayer>, "WPlayer");
  manager->setPlayerType("WPlayer");
}

void WGame::initializeClient() {
  addEntityConstructors(getWorld()->getNetworkManager());
  game->worldspawn = 0;

  world->getPhysicsWorld()->getWorld()->setGravity(btVector3(0, 0, -106.67));

  std::scoped_lock lock(world->worldLock);
  world->stepped.listen([this] {
    if (!game->worldspawn)
      game->worldspawn =
          (Worldspawn*)world->getNetworkManager()->findEntityByType(
              "Worldspawn");

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

  game->state = MainMenu;
  gfxEngine->renderStepped.listen([this] {
    Input::singleton()->setMouseLocked(false);
    network::Peer::Type peerType =
        world->getNetworkManager()->getLocalPeer().type;
    if (peerType == network::Peer::ConnectedPlayer) {
      if (game->worldspawn) {
        if (game->worldspawn->isPendingAddToGfx()) {
          game->worldspawn->addToEngine(gfxEngine.get());
        }
        Input::singleton()->setMouseLocked(true);
        game->worldspawn->getFile()->updatePosition(
            gfxEngine->getCamera().getPosition());
      } else {
        ImGui::Begin("Connecting to server...");
        ImGui::Text("Waiting for worldspawn");
        ImGui::End();
      }
    } else {
      if (peerType == network::Peer::Undifferentiated) {
        ImGui::Begin("Connecting to server...");
        ImGui::Text("Establishing connection...");
        ImGui::End();
      } else {
        switch (game->state) {
          case MainMenu:
            ImGui::Begin("Welcome to RDM");
            if (ImGui::Button("Connect to Game")) {
              game->state = ConnectPanel;
            }
            if (ImGui::Button("Quit")) {
              InputObject quitObject;
              quitObject.type = InputObject::Quit;
              Input::singleton()->postEvent(quitObject);
            }
            ImGui::End();
            break;
          case ConnectPanel:
            ImGui::Begin("Connect to Server");
            {
              static char ip[64] = "127.0.0.1";
              static int port = 7938;
              ImGui::InputText("IP", ip, sizeof(ip));
              ImGui::InputInt("Port", &port);
              if (ImGui::Button("Connect")) {
                world->getNetworkManager()->connect(ip, port);
                game->state = MainMenu;
              }
              if (ImGui::Button("Cancel")) {
                game->state = MainMenu;
              }
            }
            ImGui::End();
          default:
            break;
        }
      }
    }
  });
}

void WGame::initializeServer() {
  addEntityConstructors(worldServer->getNetworkManager());
  worldServer->getNetworkManager()->start();
  worldServer->getPhysicsWorld()->getWorld()->setGravity(
      btVector3(0, 0, -106.67));

  Worldspawn* wspawn =
      (Worldspawn*)worldServer->getNetworkManager()->instantiate("Worldspawn");
  wspawn->loadFile("dat5/baseq3/maps/chud.bsp");
}

void WGame::initialize() {
  if (rdm::Settings::singleton()->getHintDs()) {
    startServer();
  } else {
    startClient();
  }
}
}  // namespace ww
