#pragma once
#include <glm/glm.hpp>
#include <mutex>

namespace rdm::gfx {
class BaseContext {
  void* hwnd;

  std::mutex mutex;
public:
  BaseContext(void* hwnd);
  virtual ~BaseContext() {};

  std::mutex& getMutex() { return mutex; }

  // YOU should lock mutex
  virtual void setCurrent() = 0;
  virtual void swapBuffers() = 0;
  // YOU should unlock mutex
  virtual void unsetCurrent() = 0;
  virtual glm::ivec2 getBufferSize() = 0;

  void* getHwnd() { return hwnd; }
};
};