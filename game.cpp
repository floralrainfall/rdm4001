#include "game.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_video.h>

#include <cstdlib>
#include <stdexcept>

#include "gfx/gl_context.hpp"
#include "input.hpp"
#include "logging.hpp"
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
Game::Game() {
  Log::singleton()->setLevel(Settings::singleton()->getSetting("LogLevel", 1));
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

  gfxEngine.reset(new gfx::Engine(world.get(), (void*)window));
  ImGui::SetCurrentContext(ImGui::CreateContext());
  ImGui_ImplSDL2_InitForOpenGL(
      window, ((gfx::gl::GLContext*)gfxEngine->getContext())->getContext());
  gfxEngine->getContext()->unsetCurrent();

  world->getNetworkManager()->setGfxEngine(gfxEngine.get());
  world->changingTitle.listen(
      [this](std::string title) { SDL_SetWindowTitle(window, title.c_str()); });
}

void Game::startServer() { worldServer.reset(new World()); }

void Game::mainLoop() {
  try {
    initialize();

    if (world) {
      initializeClient();
      world->getScheduler()->startAllJobs();
    }
    if (worldServer) {
      initializeServer();
      worldServer->changingTitle.listen(
          [](std::string title) { printf("\033]0;%s\007", title.c_str()); });
      worldServer->getScheduler()->startAllJobs();
    }
  } catch (std::exception& e) {
    Log::printf(LOG_FATAL, "Error initializing game: %s", e.what());
    exit(EXIT_FAILURE);
  }

  if (world) {
    SDL_Event event;
    bool ignoreNextMouseMoveEvent = false;
    while (world->getRunning()) {
#ifndef DISABLE_EASY_PROFILER
      EASY_BLOCK("SDL_PollEvent");
#endif
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
            if (Input::singleton()->getMouseLocked()) {
              int flags = SDL_GetWindowFlags(window);
              if (!(flags & SDL_WINDOW_INPUT_FOCUS)) {
                SDL_ShowCursor(true);
                break;
              }
            }

            if (ignoreNextMouseMoveEvent) {
              ignoreNextMouseMoveEvent = false;
              break;
            }
            object.type = InputObject::MouseMove;
            object.data.mouse.delta[0] = event.motion.xrel;
            object.data.mouse.delta[1] = event.motion.yrel;
            SDL_ShowCursor(!Input::singleton()->getMouseLocked());
            if (Input::singleton()->getMouseLocked()) {
              int w, h;
              SDL_GetWindowSize(window, &w, &h);
              SDL_WarpMouseInWindow(window, w / 2, h / 2);
              object.data.mouse.position[0] = w / 2;
              object.data.mouse.position[1] = w / 2;
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
      std::this_thread::yield();
    }
  }
  if (world) world->getScheduler()->waitToWrapUp();
  if (worldServer) worldServer->getScheduler()->waitToWrapUp();
}
}  // namespace rdm
