#pragma once
#include <SDL2/SDL.h>

#include <mutex>

#include "base_context.hpp"

namespace rdm::gfx::gl {
class GLContext : public BaseContext {
  SDL_GLContext context;

 public:
  GLContext(void* hwnd);

  virtual void setCurrent();
  virtual void unsetCurrent();
  virtual void swapBuffers();
  virtual glm::ivec2 getBufferSize();
};
};  // namespace rdm::gfx::gl