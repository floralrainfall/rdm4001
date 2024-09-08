#pragma once
#include "gl_context.hpp"
#include "base_device.hpp"
#include <glad/glad.h>

namespace rdm::gfx {
class GLDevice : public BaseDevice {
public:
  GLDevice(GLContext* context);

  virtual void clear(float r, float g, float b, float a);
  virtual void viewport(int x, int y, int w, int h);

  static GLenum drawType(DrawType dtype);

  virtual void draw(BaseBuffer* base, DataType type, DrawType dtype, size_t count, void* pointer = 0);

  virtual std::unique_ptr<BaseTexture> createTexture();
  virtual std::unique_ptr<BaseProgram> createProgram();
  virtual std::unique_ptr<BaseBuffer> createBuffer();
  virtual std::unique_ptr<BaseArrayPointers> createArrayPointers();
};
};