#include "gl_device.hpp"

#include <glad/glad.h>

#include <stdexcept>

#include "gl_types.hpp"

namespace rdm::gfx::gl {
GLDevice::GLDevice(GLContext* context) : BaseDevice(context) {
  currentFrameBuffer = 0;
}

GLenum dssMapping[] = {GL_NEVER,  GL_ALWAYS,  GL_NEVER,    GL_LESS,  GL_EQUAL,
                       GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL};

void GLDevice::setDepthState(DepthStencilState s) {
  if (s == Disabled) {
    glDisable(GL_DEPTH_TEST);
  } else {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(dssMapping[s]);
  }
}

void GLDevice::setStencilState(DepthStencilState s) {}

GLenum bsMapping[] = {
    0,
    GL_ZERO,
    GL_ONE,
    GL_SRC_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_DST_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_CONSTANT_COLOR,
    GL_ONE_MINUS_CONSTANT_COLOR,
    GL_CONSTANT_ALPHA,
    GL_ONE_MINUS_CONSTANT_ALPHA,
};

void GLDevice::setBlendState(BlendState a, BlendState b) {
  if (a == DDisabled || b == DDisabled) {
    glDisable(GL_BLEND);
  } else {
    glEnable(GL_BLEND);
    glBlendFunc(bsMapping[a], bsMapping[b]);
  }
}

void GLDevice::clear(float r, float g, float b, float a) {
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}

void GLDevice::clearDepth(float d) {
  glClearDepth(d);
  glClear(GL_DEPTH_BUFFER_BIT);
}

void GLDevice::viewport(int x, int y, int w, int h) { glViewport(x, y, w, h); }

GLenum GLDevice::drawType(DrawType type) {
  switch (type) {
    default:
      return GL_TRIANGLES;
  }
}

void GLDevice::draw(BaseBuffer* base, DataType type, DrawType dtype,
                    size_t count, void* pointer) {
  base->bind();
  switch (dynamic_cast<GLBuffer*>(base)->getType()) {
    case BaseBuffer::Element:
      glDrawElements(drawType(dtype), count, fromDataType(type), pointer);
      break;
    case BaseBuffer::Array:
      glDrawArrays(drawType(dtype), (GLint)(size_t)pointer, count);
      break;
    case BaseBuffer::Unknown:
    default:
      throw std::runtime_error("Bad buffer type");
  }
}

void* GLDevice::bindFramebuffer(BaseFrameBuffer* buffer) {
  void* oldFb = (void*)currentFrameBuffer;
  glBindFramebuffer(GL_FRAMEBUFFER, ((GLFrameBuffer*)buffer)->getId());
  currentFrameBuffer = buffer;
  return oldFb;
}

void GLDevice::unbindFramebuffer(void* p) {
  GLFrameBuffer* oldFb = (GLFrameBuffer*)p;
  glBindFramebuffer(GL_FRAMEBUFFER, oldFb ? oldFb->getId() : 0);
  currentFrameBuffer = oldFb;
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

std::unique_ptr<BaseFrameBuffer> GLDevice::createFrameBuffer() {
  return std::unique_ptr<BaseFrameBuffer>(new GLFrameBuffer());
}
}  // namespace rdm::gfx::gl