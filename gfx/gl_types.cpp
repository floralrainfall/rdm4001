#include "gl_types.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>

#include "gfx/base_types.hpp"
#include "glad/glad.h"
#include "logging.hpp"

namespace rdm::gfx::gl {
GLenum fromDataType(DataType t) {
  switch (t) {
    case DtUnsignedByte:
      return GL_UNSIGNED_BYTE;
    case DtByte:
      return GL_BYTE;
    case DtUnsignedShort:
      return GL_UNSIGNED_SHORT;
    case DtShort:
      return GL_SHORT;
    case DtUnsignedInt:
      return GL_UNSIGNED_INT;
    case DtInt:
      return GL_INT;
    case DtFloat:
      return GL_FLOAT;
    default:
      throw std::runtime_error("Invalid type");
  }
}

GLTexture::GLTexture() { glGenTextures(1, &texture); }

GLTexture::~GLTexture() { glDeleteTextures(1, &texture); }

GLenum GLTexture::texType(BaseTexture::Type type) {
  switch (type) {
    case Texture2D:
      return GL_TEXTURE_2D;
      break;
    default:
      throw std::runtime_error("Invalid type");
  }
}

GLenum GLTexture::texFormat(Format format) {
  switch (format) {
    case RGB:
      return GL_RGB;
    case RGBA:
      return GL_RGBA;
    default:
      throw std::runtime_error("Invalid type");
  }
}

GLenum GLTexture::texInternalFormat(InternalFormat format) {
  switch (format) {
    case RGB8:
      return GL_RGB8;
    case RGBA8:
      return GL_RGBA8;
    case D24S8:
      return GL_DEPTH24_STENCIL8;
    default:
      throw std::runtime_error("Invalid type");
  }
}

void GLTexture::reserve2d(int width, int height, InternalFormat format,
                          int mipmapLevels) {
  textureType = Texture2D;
  textureFormat = format;

  GLenum target = texType(textureType);
  glBindTexture(target, texture);
  glTexStorage2D(target, mipmapLevels, texInternalFormat(textureFormat), width,
                 height);
  if (mipmapLevels != 1) {
    glGenerateMipmap(target);
  }
  glBindTexture(target, 0);
}

void GLTexture::upload2d(int width, int height, DataType type,
                         BaseTexture::Format format, void* data,
                         int mipmapLevels) {
  textureType = Texture2D;

  switch (format) {
    case RGB:
      textureFormat = RGB8;
      break;
    case RGBA:
      textureFormat = RGBA8;
      break;
    default:
      throw std::runtime_error("Invalid type");
  }

  GLenum target = texType(textureType);

  glBindTexture(target, texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glTexImage2D(target, mipmapLevels, texInternalFormat(textureFormat), width,
               height, 0, texFormat(format), fromDataType(type), data);
  if (mipmapLevels != 1) {
    glGenerateMipmap(target);
  }
  glBindTexture(target, 0);
}

void GLTexture::uploadCubeMap(int width, int height, std::vector<void*> data) {
  textureType = CubeMap;
  GLenum target = texType(textureType);
  glBindTexture(target, texture);
  if(data.size() == 6) {
    throw std::runtime_error("std::vector<void*> data.size() must be equal to 6, fill unchanged elements with NULL");
  }

  for(void* p : data) {

  }
  
  glBindTexture(target, 0);
}

void GLTexture::destroyAndCreate() {
  glDeleteTextures(1, &texture);
  glGenTextures(1, &texture);
}

void GLTexture::bind() {
  GLenum target = texType(textureType);
  glBindTexture(target, texture);
}

GLProgram::GLProgram() { program = glCreateProgram(); }

GLProgram::~GLProgram() { glDeleteProgram(program); }

GLenum GLProgram::shaderType(Shader type) {
  switch (type) {
    case Vertex:
      return GL_VERTEX_SHADER;
    case Fragment:
      return GL_FRAGMENT_SHADER;
    case Geometry:
      return GL_GEOMETRY_SHADER;
    default:
      break;
  }
}

void GLProgram::link() {
  std::vector<GLuint> _shaders;
  std::string programName;
  for (auto [type, shader] : shaders) {
    Log::printf(LOG_INFO, "Compiling shader %s", shader.name.c_str());

    GLuint _shader = glCreateShader(shaderType(type));
    glObjectLabel(GL_SHADER, _shader, shader.name.size(), shader.name.data());
    programName += shader.name + " ";
    GLchar* code = (GLchar*)shader.code.c_str();
    int codeLength[] = {(int)shader.code.size()};
    glShaderSource(_shader, 1, &code, (const GLint*)&codeLength);
    glCompileShader(_shader);

    GLint success = 0;
    glGetShaderiv(_shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
      GLint logSize = 0;
      glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &logSize);
      char* infoLog = (char*)malloc(logSize);
      glGetShaderInfoLog(_shader, logSize, NULL, infoLog);
      Log::printf(LOG_ERROR, "Shader compile %s error\n%s", shader.name.c_str(),
                  infoLog);
      Log::printf(LOG_DEBUG, "Shader code:\n%s", shader.code.c_str());
      free(infoLog);

      throw std::runtime_error("Shader compile error");
    } else {
      Log::printf(LOG_DEBUG, "Successfully compiled shader %s",
                  shader.name.c_str());
    }

    glAttachShader(program, _shader);
    _shaders.push_back(_shader);
  }

  glObjectLabel(GL_PROGRAM, program, programName.size(), programName.data());
  glLinkProgram(program);

  GLint success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (success == GL_FALSE) {
    GLint logSize = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
    char* infoLog = (char*)malloc(logSize);
    glGetProgramInfoLog(program, logSize, NULL, infoLog);
    Log::printf(LOG_ERROR, "Program link error\n%s", infoLog);
    free(infoLog);

    throw std::runtime_error("Program link error");
  }

  for (auto shader : _shaders) {
    glDeleteShader(shader);
  }
}

void GLProgram::bindParameters() {
  for (auto& [name, pair] : parameters) {
    GLuint object = glGetUniformLocation(program, name.c_str());
    switch (pair.first.type) {
      case DtInt:
	glUniform1i(object, pair.second.integer);
	break;
      case DtMat2:
        glUniformMatrix2fv(object, 1, false,
                           glm::value_ptr(pair.second.matrix2x2));
        break;
      case DtMat3:
        glUniformMatrix3fv(object, 1, false,
                           glm::value_ptr(pair.second.matrix3x3));
        break;
      case DtMat4:
        glUniformMatrix4fv(object, 1, false,
                           glm::value_ptr(pair.second.matrix4x4));
        break;
      case DtVec2:
        glUniform2fv(object, 1, glm::value_ptr(pair.second.vec2));
        break;
      case DtVec3:
        glUniform3fv(object, 1, glm::value_ptr(pair.second.vec3));
        break;
      case DtVec4:
        glUniform4fv(object, 1, glm::value_ptr(pair.second.vec4));
        break;
      case DtFloat:
        glUniform1fv(object, 1, &pair.second.number);
        break;
      case DtSampler:
        if (pair.second.texture.texture) {
          glActiveTexture(GL_TEXTURE0 + pair.second.texture.slot);
          pair.second.texture.texture->bind();
          glUniform1i(object, pair.second.texture.slot);
        }
        break;
      default:
        throw std::runtime_error("FIX THIS!! bad datatype for parameter");
        break;
    }
    pair.first.dirty = false;
  }
}

void GLProgram::bind() {
  glUseProgram(program);
  bindParameters();
}

GLBuffer::GLBuffer() { glCreateBuffers(1, &buffer); }

GLBuffer::~GLBuffer() { glDeleteBuffers(1, &buffer); }

GLenum GLBuffer::bufType(Type type) {
  switch (type) {
    case Element:
      return GL_ELEMENT_ARRAY_BUFFER;
    case Array:
      return GL_ARRAY_BUFFER;
    default:
      throw std::runtime_error("GL bufType(Type type) used with bad type");
  }
}

GLenum GLBuffer::bufUsage(Usage usage) {
  switch (usage) {
    case StaticDraw:
      return GL_STATIC_DRAW;
    default:
      return GL_DYNAMIC_DRAW;
  }
}

void GLBuffer::upload(Type type, Usage usage, size_t size, const void* data) {
  this->type = type;
  glBindBuffer(bufType(type), buffer);
  glBufferData(bufType(type), size, data, bufUsage(usage));
  glBindBuffer(bufType(type), 0);
}

void GLBuffer::bind() { glBindBuffer(bufType(type), buffer); }

GLArrayPointers::GLArrayPointers() { glCreateVertexArrays(1, &array); }

GLArrayPointers::~GLArrayPointers() { glDeleteVertexArrays(1, &array); }

void GLArrayPointers::upload() {
  glBindVertexArray(array);
  for (auto attrib : attribs) {
    if (attrib.buffer &&
        dynamic_cast<GLBuffer*>(attrib.buffer)->getType() == GLBuffer::Array) {
      attrib.buffer->bind();
    }
    glEnableVertexAttribArray(attrib.layoutId);
    glVertexAttribPointer(attrib.layoutId, attrib.size,
                          fromDataType(attrib.type), attrib.normalized,
                          attrib.stride, attrib.offset);
  }
  glBindVertexArray(0);
}

void GLArrayPointers::bind() { glBindVertexArray(array); }

GLFrameBuffer::GLFrameBuffer() { glGenFramebuffers(1, &framebuffer); }

GLFrameBuffer::~GLFrameBuffer() { glDeleteFramebuffers(1, &framebuffer); }

void GLFrameBuffer::setTarget(BaseTexture* texture, AttachmentPoint point) {
  GLTexture* _gltexture = dynamic_cast<GLTexture*>(texture);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  GLenum atp;
  switch (point) {
    case Depth:
      atp = GL_DEPTH_ATTACHMENT;
      break;
    case Stencil:
      atp = GL_STENCIL_ATTACHMENT;
      break;
    case DepthStencil:
      atp = GL_DEPTH_STENCIL_ATTACHMENT;
      break;
    default:  // should be colors
      atp = GL_COLOR_ATTACHMENT0 + point;
      break;
  }
  glFramebufferTexture2D(GL_FRAMEBUFFER, atp,
                         GLTexture::texType(texture->getType()),
                         _gltexture->getId(), 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

BaseFrameBuffer::Status GLFrameBuffer::getStatus() {
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  switch (glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    default:
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      return BadAttachment;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
      return BadDimensions;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      return MissingAttachment;
    case GL_FRAMEBUFFER_COMPLETE:
      return Complete;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      return Unsupported;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFrameBuffer::destroyAndCreate() {
  glDeleteFramebuffers(1, &framebuffer);
  glCreateFramebuffers(1, &framebuffer);
}
}  // namespace rdm::gfx::gl
