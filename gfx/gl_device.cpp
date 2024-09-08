#include "gl_device.hpp"
#include "gl_types.hpp"
#include <glad/glad.h>
#include <stdexcept>

namespace rdm::gfx {
GLDevice::GLDevice(GLContext* context) : BaseDevice(context) {

}

void GLDevice::clear(float r, float g, float b, float a) {
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLDevice::viewport(int x, int y, int w, int h) {
  glViewport(x, y, w, h);
}

GLenum GLDevice::drawType(DrawType type) {
  switch(type) {
  default:
    return GL_TRIANGLES;
  }
}

void GLDevice::draw(BaseBuffer* base, DataType type, DrawType dtype, size_t count, void* pointer) {
  base->bind();
  switch(dynamic_cast<GLBuffer*>(base)->getType()) {
  case BaseBuffer::Element:
    glDrawElements(drawType(dtype), count, fromDataType(type), pointer);
    break;
  default:
    throw std::runtime_error("Bad buffer type");
  }
}

std::unique_ptr<BaseTexture> GLDevice::createTexture() {
  return std::unique_ptr<BaseTexture>(new GLTexture());
}

std::unique_ptr<BaseProgram> GLDevice::createProgram() {
  return std::unique_ptr<BaseProgram>(new GLProgram());
}

std::unique_ptr<BaseBuffer> GLDevice::createBuffer() {
  return std::unique_ptr<BaseBuffer>(new GLBuffer());
}

std::unique_ptr<BaseArrayPointers> GLDevice::createArrayPointers() {
  return std::unique_ptr<BaseArrayPointers>(new GLArrayPointers());
}
}