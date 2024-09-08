#include "game.hpp"
#include "guid.hpp"
#include <SDL2/SDL.h>
#include "logging.hpp"
#include "input.hpp"
#include <easy/profiler.h>
#include <stdexcept>
#ifndef DISABLE_OBZ
#include "obz.hpp"
#else 
#warning Compiled without OBZ support.
#endif

namespace rdm {
  Game::Game() {
#ifndef DISABLE_PROFILER
    EASY_MAIN_THREAD;
    profiler::startListen();
#endif

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
      if(!window)
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

    window = SDL_CreateWindow("A rdm presentation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);

#ifndef DISABLE_OBZ
    obz::ObzFileSystemAPI* obzFsApi;
    try {
      obzFsApi = dynamic_cast<obz::ObzFileSystemAPI*>(common::FileSystem::singleton()->addApi(std::unique_ptr<obz::ObzFileSystemAPI>(new obz::ObzFileSystemAPI("game")), true));
    } catch(std::runtime_error& e) {
      Log::printf(LOG_ERROR, "Couldn't open archive what() = %s", e.what());
    }
#endif

    if(!window)
      Log::printf(LOG_FATAL, "Unable to create Window (%s)", SDL_GetError());
    world = std::unique_ptr<World>(new World());
#ifndef DISABLE_OBZ
    if(obzFsApi)
      obzFsApi->addToScheduler(world->getScheduler());
#endif

    gfxEngine = std::unique_ptr<gfx::Engine>(new gfx::Engine(world.get(), (void*)window));
    gfxEngine->getContext()->unsetCurrent();
  }

  void Game::mainLoop() {
    initialize();

    world->getScheduler()->startAllJobs();
    SDL_Event event;
    while(world->getRunning()) {
      EASY_BLOCK("Poll Events");
      while(SDL_PollEvent(&event)) {
        InputObject object;
        switch (event.type)
        {
        case SDL_KEYUP:
        case SDL_KEYDOWN:
          object.type = event.type == SDL_KEYDOWN ? InputObject::KeyPress : InputObject::KeyUp;
          object.data.key.key = event.key.keysym.sym;
          Input::singleton()->postEvent(object);
          break;
        case SDL_QUIT:
          object.type = InputObject::Quit;
          Input::singleton()->postEvent(object);
          break;
        default:
          break;
        }
      }
      EASY_END_BLOCK;

      std::this_thread::yield();
    }
    world->getScheduler()->waitToWrapUp();
  }
}