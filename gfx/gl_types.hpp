#pragma once
#include "base_types.hpp"
#include <glad/glad.h>

namespace rdm::gfx {
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

  virtual void upload2d(int width, int height, DataType type, Format format, void* data, int mipmapLevels = 0);
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
}