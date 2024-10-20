#pragma once
#include <glad/glad.h>

#include "base_types.hpp"

namespace rdm::gfx::gl {
GLenum fromDataType(DataType t);

class GLTexture : public BaseTexture {
  GLuint texture;

 public:
  GLTexture();
  virtual ~GLTexture();

  GLuint getId() { return texture; }

  static GLenum texType(Type type);
  static GLenum texFormat(Format format);
  static GLenum texInternalFormat(InternalFormat format);

  virtual void reserve2d(int width, int height, InternalFormat format,
                         int mipmapLevels);
  virtual void upload2d(int width, int height, DataType type, Format format,
                        void* data, int mipmapLevels);
  virtual void uploadCubeMap(int width, int height, std::vector<void*> data);
  virtual void destroyAndCreate();
  virtual void bind();
};

class GLProgram : public BaseProgram {
  GLuint program;

 public:
  GLProgram();
  virtual ~GLProgram();

  GLuint getId() { return program; }

  static GLenum shaderType(Shader type);

  void bindParameters();

  virtual void bind();
  virtual void link();
};

class GLBuffer : public BaseBuffer {
  GLuint buffer;
  Type type;

 public:
  GLBuffer();
  virtual ~GLBuffer();

  static GLenum bufType(Type type);
  static GLenum bufUsage(Usage usage);

  Type getType() { return type; }

  virtual void upload(Type type, Usage usage, size_t size, const void* data);
  virtual void bind();
};

class GLArrayPointers : public BaseArrayPointers {
  GLuint array;

 public:
  GLArrayPointers();
  virtual ~GLArrayPointers();

  virtual void upload();
  virtual void bind();
};

class GLFrameBuffer : public BaseFrameBuffer {
  GLuint framebuffer;

 public:
  GLFrameBuffer();
  ~GLFrameBuffer();

  virtual void setTarget(BaseTexture* texture, AttachmentPoint point);
  virtual Status getStatus();
  virtual void destroyAndCreate();

  GLuint getId() { return framebuffer; }
};
}  // namespace rdm::gfx::gl
