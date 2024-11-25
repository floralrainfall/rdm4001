#include "gl_context.hpp"

#include <glad/glad.h>

#include <stdexcept>

#include "SDL_video.h"
#include "logging.hpp"
#include "settings.hpp"

#ifndef DISABLE_EASY_PROFILER
#include <easy/profiler.h>
#endif

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar* message, const void* userParam) {
  rdm::Log::printf(
      type == GL_DEBUG_TYPE_ERROR ? rdm::LOG_ERROR : rdm::LOG_EXTERNAL,
      "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
      (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity,
      message);
  if (type == GL_DEBUG_TYPE_ERROR) throw std::runtime_error(message);
}

namespace rdm::gfx::gl {
// we expect SDL's gl context
static CVar r_gldebug("r_gldebug", "0", CVARF_SAVE);
static CVar r_glvsync("r_glvsync", "0", CVARF_SAVE);

GLContext::GLContext(void* hwnd) : BaseContext(hwnd) {
  context = SDL_GL_CreateContext((SDL_Window*)hwnd);
  if (!context)
    Log::printf(LOG_FATAL, "Unable to create GLContext (%s)", SDL_GetError());

  setCurrent();
  if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
    Log::printf(LOG_FATAL, "Unable to initialize GLAD");
  }
  SDL_GL_SetSwapInterval(r_glvsync.getBool());
  r_glvsync.changing.listen(
      [] { SDL_GL_SetSwapInterval(r_glvsync.getBool()); });

  if (r_gldebug.getBool()) {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(MessageCallback, 0);
  }

  Log::printf(LOG_INFO, "Vendor:   %s", glGetString(GL_VENDOR));
  Log::printf(LOG_INFO, "Renderer: %s", glGetString(GL_RENDERER));
  Log::printf(LOG_INFO, "Version:  %s", glGetString(GL_VERSION));
}

void GLContext::swapBuffers() {
#ifndef DISABLE_EASY_PROFILER
  EASY_FUNCTION();
#endif
  SDL_GL_SwapWindow((SDL_Window*)getHwnd());
}

void GLContext::setCurrent() {
#ifndef DISABLE_EASY_PROFILER
  EASY_FUNCTION();
#endif
  if (SDL_GL_MakeCurrent((SDL_Window*)getHwnd(), context)) {
    Log::printf(LOG_FATAL, "Unable to make context current (%s)",
                SDL_GetError());
  }
}

void GLContext::unsetCurrent() {
#ifndef DISABLE_EASY_PROFILER
  EASY_FUNCTION();
#endif
  SDL_GL_MakeCurrent((SDL_Window*)getHwnd(), NULL);
}

glm::ivec2 GLContext::getBufferSize() {
  glm::ivec2 z;
  SDL_GetWindowSize((SDL_Window*)getHwnd(), &z.x, &z.y);
  return z;
}
}  // namespace rdm::gfx::gl
