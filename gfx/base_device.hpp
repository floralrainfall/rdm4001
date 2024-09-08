#pragma once
#include "base_context.hpp"
#include "base_types.hpp"
#include <memory>

namespace rdm::gfx {
class BaseDevice {
  BaseContext* context;
public:
  BaseDevice(BaseContext* context);
  virtual ~BaseDevice() {};
  
  virtual void clear(float r, float g, float b, float a) = 0;
  virtual void viewport(int x, int y, int w, int h) = 0;

  enum DrawType {
    Points,
    Lines,
    Triangles,
  };

  virtual void draw(BaseBuffer* base, DataType type, DrawType dtype, size_t count, void* pointer = 0) = 0;

  virtual std::unique_ptr<BaseTexture> createTexture() = 0;
  virtual std::unique_ptr<BaseProgram> createProgram() = 0;
  virtual std::unique_ptr<BaseBuffer> createBuffer() = 0;
  virtual std::unique_ptr<BaseArrayPointers> createArrayPointers() = 0;
};
};