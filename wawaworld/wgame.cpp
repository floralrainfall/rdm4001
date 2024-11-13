#include "wgame.hpp"

#include "SDL_keycode.h"
#include "filesystem.hpp"
#include "gfx/base_types.hpp"
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
#include "sound.hpp"
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

  std::unique_ptr<SoundEmitter> mainMenuSound;
};

using namespace rdm;
WGame::WGame() : Game() {
  Input::singleton()->newAxis("ForwardBackward", SDLK_w, SDLK_s);
  Input::singleton()->newAxis("LeftRight", SDLK_a, SDLK_d);

  game = new WGamePrivate();
}

size_t WGame::getGameVersion() { return 0x00000001; }

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

  world->getPhysicsWorld()->getWorld()->setGravity(btVector3(0, 0, -206.67));

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

  Input::singleton()->keyDownSignals[SDLK_TAB].listen([] {
    Input::singleton()->setMouseLocked(!Input::singleton()->getMouseLocked());
    Log::printf(LOG_DEBUG, "mouseLocked = %s",
                Input::singleton()->getMouseLocked() ? "true" : "false");
  });

  gfxEngine->initialized.listen([this] {
    // game->file->initGfx(gfxEngine.get());
    // gfxEngine->addEntity<MapEntity>(game->file);
  });

  game->mainMenuSound.reset(getSoundManager()->newEmitter());
  /*game->mainMenuSound->play(getSoundManager()
                                ->getSoundCache()
                                ->get("dat5/main_menu.ogg", Sound::Stream)
                                .value());*/

  game->state = MainMenu;
  gfxEngine->renderStepped.listen([this] {
    network::Peer::Type peerType =
        world->getNetworkManager()->getLocalPeer().type;
    if (worldServer) {
      ImGui::Begin("Server");
      ImGui::Text("Currently hosting");
      ImGui::End();
    }

    ImGui::Begin("Scheduler");
    ImGui::Text("Client");
    world->getScheduler()->imguiDebug();
    if (worldServer) {
      ImGui::Separator();
      ImGui::Text("Server");
      worldServer->getScheduler()->imguiDebug();
    }
    ImGui::End();

    if (peerType == network::Peer::ConnectedPlayer) {
      if (game->worldspawn) {
        if (game->worldspawn->isPendingAddToGfx()) {
          game->worldspawn->addToEngine(gfxEngine.get());
          Input::singleton()->setMouseLocked(true);
        }
        if (game->worldspawn->getFile())
          game->worldspawn->getFile()->updatePosition(
              gfxEngine->getCamera().getPosition());

        glm::ivec2 size = gfxEngine->getContext()->getBufferSize();
        /*ImGui::SetNextWindowSize(ImVec2(size.x, size.y));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin(
            "HUD", NULL,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);

            ImGui::End();*/
      } else {
        ImGui::Begin("Connecting to server...");
        ImGui::Text("Waiting for worldspawn");
        ImGui::End();
      }
    } else {
      Input::singleton()->setMouseLocked(false);
      if (peerType == network::Peer::Undifferentiated) {
        ImGui::Begin("Connecting to server...");
        ImGui::Text("Establishing connection...");
        ImGui::End();
      } else {
        switch (game->state) {
          case MainMenu: {
            glm::ivec2 size = gfxEngine->getContext()->getBufferSize();
            ImGui::SetNextWindowSize(ImVec2(size.x, size.y));
            ImGui::SetNextWindowPos(ImVec2(0, 0));
          }
            ImGui::Begin("Welcome to RDM", NULL,
                         ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoBringToFrontOnFocus);

            {
              gfx::BaseTexture* logo = gfxEngine->getTextureCache()
                                           ->getOrLoad2d("dat5/logo.png")
                                           .value()
                                           .second;
              ImGui::Image(logo->getImTextureId(), ImVec2(540, 451), {0, 1},
                           {1, 0});
            }

            if (ImGui::Button("Connect to Game")) {
              game->state = ConnectPanel;
            }
            if (ImGui::Button("Host")) {
              lateInitServer();
              world->getNetworkManager()->connect("127.0.0.1", 7938);
            }
            if (ImGui::Button("Quit")) {
              InputObject quitObject;
              quitObject.type = InputObject::Quit;
              Input::singleton()->postEvent(quitObject);
            }

            ImGui::Text("RDM %08x, Engine %08x", getGameVersion(),
                        getVersion());
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
  wspawn->loadFile("dat5/baseq3/maps/ffa_naamda.bsp");
}

void WGame::initialize() {
  if (rdm::Settings::singleton()->getHintDs()) {
    startServer();
  } else {
    startClient();
  }
}
}  // namespace ww
