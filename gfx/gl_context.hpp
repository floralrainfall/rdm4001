#pragma once
#include "base_context.hpp"
#include <SDL2/SDL.h>
#include <mutex>

namespace rdm::gfx {
class GLContext : public BaseContext {
  SDL_GLContext context;
public:
  GLContext(void* hwnd);

  virtual void setCurrent();
  virtual void unsetCurrent();
  virtual void swapBuffers();
  virtual glm::ivec2 getBufferSize();
};
};