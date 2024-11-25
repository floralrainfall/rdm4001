#include "game.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_video.h>

#include <chrono>
#include <cstdlib>
#include <stdexcept>

#include "fun.hpp"
#include "gfx/gl_context.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "network/network.hpp"
#include "scheduler.hpp"
#ifndef DISABLE_OBZ
#include "obz.hpp"
#else
#warning Compiled without OBZ support.
#endif
#include "settings.hpp"

#ifdef __linux
#include <signal.h>
#endif

#ifndef DISABLE_EASY_PROFILER
#include <easy/profiler.h>
#endif

#include "gfx/imgui/backends/imgui_impl_sdl2.h"
#include "gfx/imgui/imgui.h"

namespace rdm {
static CVar cl_copyright("cl_copyright", "1", CVARF_SAVE);
static CVar cl_loglevel("cl_loglevel", "2", CVARF_SAVE);

Game::Game() {
  if (!Fun::preFlightChecks()) abort();  // clearly not safe to run

  ignoreNextMouseMoveEvent = false;

  network::NetworkManager::initialize();

  Log::singleton()->setLevel((LogType)cl_loglevel.getInt());
  cl_loglevel.changing.listen(
      [] { Log::singleton()->setLevel((LogType)cl_loglevel.getInt()); });

  if (cl_copyright.getBool()) Log::printf(LOG_INFO, "%s", copyright());
}

Game::~Game() { network::NetworkManager::deinitialize(); }

size_t Game::getVersion() { return ENGINE_VERSION; }

const char* Game::copyright() {
  return R"a(RDM4001, a 3D game engine
Copyright (C) 2024-2025 Entropy Interactive

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.)a";
}

void Game::startClient() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    if (!window)
      Log::printf(LOG_FATAL, "Unable to init SDL (%s)", SDL_GetError());
  }

  int context_flags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
#ifndef NDEBUG
  context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_LoadLibrary(NULL);

  window = SDL_CreateWindow("A rdm presentation", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 800, 600,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

#ifndef DISABLE_OBZ
  obz::ObzFileSystemAPI* obzFsApi = 0;
  try {
    obzFsApi = dynamic_cast<obz::ObzFileSystemAPI*>(
        common::FileSystem::singleton()->addApi(
            std::unique_ptr<obz::ObzFileSystemAPI>(
                new obz::ObzFileSystemAPI("game")),
            true));
  } catch (std::runtime_error& e) {
    Log::printf(LOG_ERROR, "Couldn't open archive what() = %s", e.what());
  }
#endif

  if (!window)
    Log::printf(LOG_FATAL, "Unable to create Window (%s)", SDL_GetError());
  world.reset(new World());
#ifndef DISABLE_OBZ
  if (obzFsApi) obzFsApi->addToScheduler(world->getScheduler());
#endif

  soundManager.reset(new SoundManager(world.get()));

  gfxEngine.reset(new gfx::Engine(world.get(), (void*)window));
  ImGui::SetCurrentContext(ImGui::CreateContext());
  ImGui_ImplSDL2_InitForOpenGL(
      window, ((gfx::gl::GLContext*)gfxEngine->getContext())->getContext());
  gfxEngine->getContext()->unsetCurrent();

  world->getNetworkManager()->setGfxEngine(gfxEngine.get());
  world->getNetworkManager()->setGame(this);
  world->changingTitle.listen(
      [this](std::string title) { SDL_SetWindowTitle(window, title.c_str()); });
}

void Game::startServer() {
  worldServer.reset(new World());
  worldServer->getNetworkManager()->setGame(this);
}

void Game::lateInitServer() {
  Log::printf(LOG_INFO, "Starting built-in server");
  startServer();
  initializeServer();
  worldServer->getScheduler()->startAllJobs();
}

static CVar input_rate("input_rate", "20.0", CVARF_SAVE);
static CVar sv_ansi("sv_ansi", "1", CVARF_SAVE);

class GameEventJob : public SchedulerJob {
  Game* game;

 public:
  GameEventJob(Game* game) : SchedulerJob("GameEvent") { this->game = game; }

  virtual Result step() {
    game->pollEvents();
    return Stepped;
  }

  virtual double getFrameRate() { return 1.0 / input_rate.getFloat(); }
};

void Game::earlyInit() {
  try {
    initialize();

    if (world) {
      initializeClient();

      world->getScheduler()->addJob(new GameEventJob(this));
      world->getScheduler()->startAllJobs();
    }
    if (worldServer) {
      initializeServer();
      if (sv_ansi.getBool()) {
        worldServer->changingTitle.listen([](std::string title) {
          fprintf(stderr, "\033]0;%s\007", title.c_str());
        });
      }
      worldServer->getScheduler()->startAllJobs();
    }
  } catch (std::exception& e) {
    Log::printf(LOG_FATAL, "Error initializing game: %s", e.what());
    exit(EXIT_FAILURE);
  }
}

void Game::stopServer() { worldServer.reset(); }

void Game::pollEvents() {
#ifndef DISABLE_EASY_PROFILER
  EASY_BLOCK("SDL_PollEvent");
#endif
  Input::singleton()->beginFrame();
  SDL_Event event;

  bool ignoreMouse = false;
  SDL_ShowCursor(!Input::singleton()->getMouseLocked());

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_NewFrame();
    ImGui_ImplSDL2_ProcessEvent(&event);
    InputObject object;
    switch (event.type) {
      case SDL_KEYUP:
      case SDL_KEYDOWN:
        object.type = event.type == SDL_KEYDOWN ? InputObject::KeyPress
                                                : InputObject::KeyUp;
        object.data.key.key = event.key.keysym.sym;
        Input::singleton()->postEvent(object);
        break;
      case SDL_QUIT:
        object.type = InputObject::Quit;
        Input::singleton()->postEvent(object);
        break;
      case SDL_MOUSEMOTION:
        if (ignoreMouse) break;
        if (ignoreNextMouseMoveEvent) {
          ignoreNextMouseMoveEvent = false;
          break;
        }
        object.type = InputObject::MouseMove;
        object.data.mouse.delta[0] = event.motion.xrel;
        object.data.mouse.delta[1] = event.motion.yrel;
        if (Input::singleton()->getMouseLocked()) {
          int w, h;
          SDL_GetWindowSize(window, &w, &h);
          SDL_WarpMouseInWindow(window, w / 2, h / 2);
          object.data.mouse.position[0] = w / 2;
          object.data.mouse.position[1] = h / 2;
          ignoreNextMouseMoveEvent = true;
        } else {
          object.data.mouse.position[0] = event.motion.x;
          object.data.mouse.position[1] = event.motion.y;
        }
        Input::singleton()->postEvent(object);
        break;
      default:
        break;
    }
  }
  /*if (Input::singleton()->getMouseLocked()) {
    SDL_SetRelativeMouseMode(
    Input::singleton()->getMouseLocked() ? SDL_TRUE : SDL_FALSE);
    }*/
}

void Game::mainLoop() {
  earlyInit();

  if (!world && !worldServer) {
    Log::printf(LOG_FATAL,
                "world or worldServer is not set, please call startClient "
                "or startServer in your Game::initialize function");
  }

  if (world) {
    while (world->getRunning()) {
      std::this_thread::yield();
    }
  }

  if (world) {
    world->getScheduler()->waitToWrapUp();
    world->getNetworkManager()->handleDisconnect();
  }
  if (worldServer) {
    worldServer->getScheduler()->waitToWrapUp();
    worldServer->getNetworkManager()->handleDisconnect();
  }

  Log::printf(LOG_DEBUG, "World no longer running");
  if (world) {
    gfxEngine->getContext()->setCurrent();
  }
}
}  // namespace rdm
