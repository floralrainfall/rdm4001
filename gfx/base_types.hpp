#pragma once
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <glm/glm.hpp>

namespace rdm::gfx {
enum DataType {
  DtUnsignedByte,
  DtByte,
  DtUnsignedShort,
  DtShort,
  DtUnsignedInt,
  DtInt,
  DtFloat,
  DtMat3,
  DtMat4,
  DtVec2,
  DtVec3,
  DtVec4,
};

class BaseTexture {
public:
  enum Type {
    Texture1D,
    Texture2D,
    Texture2D_MultiSample,
    Texture3D,
    Texture3D_MultiSample,
    CubeMap,
    Rect2D,
    Array1D,
    Array2D,
    CubeMapArray
  };

  // sized formats
  enum InternalFormat {
    RGB8,
    RGBA8,
  };

  enum Format {
    RGB,
    RGBA,
  };

  virtual ~BaseTexture() {};

  // virtual void upload1d(int width, DataType type, Format format, void* data, int mipmapLevels = 0) = 0;

  virtual void upload2d(int width, int height, DataType type, Format format, void* data, int mipmapLevels = 0) = 0;

  virtual void bind() = 0;

  Type getType() { return textureType; }
  InternalFormat getInternalFormat() { return textureFormat; }
protected:
  Type textureType;
  InternalFormat textureFormat;
};

struct ShaderFile {
  std::string code;
  std::string name;
};

class BaseProgram {
public:
  enum Shader {
    Vertex,
    Fragment,
    Geometry
  };

  union Parameter {
    unsigned char unsignedByte;
    char byte;
    unsigned int unsignedInteger;
    int integer;
    float number;
    struct {
      int slot;
      BaseTexture* texture;
    } texture;
    glm::mat3 matrix3x3;
    glm::mat4 matrix4x4;
    glm::vec3 vec3;
    glm::vec4 vec4;
  };

  struct ParameterInfo {
    DataType type;
    bool dirty;
  };

  virtual ~BaseProgram() {};

  void addShader(ShaderFile file, Shader type) { shaders[type] = file; };
  void setParameter(std::string param, DataType type, Parameter parameter);

  virtual void link() = 0;
  virtual void bind() = 0;
protected:
  std::map<Shader, ShaderFile> shaders;
  std::map<std::string, std::pair<ParameterInfo, Parameter>> parameters;
};

class BaseBuffer {
public:
  enum Type {
    Element,
    Array
  };

  enum Usage {
    StreamDraw,
    StreamRead,
    StreamCopy,

    StaticDraw,
    StaticRead,
    StaticCopy,

    DynamicDraw,
    DynamicRead,
    DynamicCopy
  };

  virtual ~BaseBuffer() {};
  
  virtual void upload(Type type, Usage usage, size_t size, const void* data) = 0;
  virtual void bind() = 0;
protected:
  std::mutex lock;
};

class BaseArrayPointers {
public:
  struct Attrib {
    int layoutId;
    int size; // 1, 2, 3, or 4
    DataType type;
    bool normalized;
    size_t stride;
    void* offset;
    BaseBuffer* buffer; // optional external buffer

    Attrib(DataType type, int id, int size, size_t stride, void* offset, BaseBuffer* buffer = 0, bool normalized = false) {
      this->type = type;
      this->layoutId = id;
      this->size = size;
      this->stride = stride;
      this->offset = offset;
      this->buffer = buffer;
      this->normalized = normalized;
    }
  };
  
  virtual ~BaseArrayPointers() {};

  void addAttrib(Attrib attrib) { attribs.push_back(attrib); };
  
  virtual void upload() = 0;
  virtual void bind() = 0;
protected:
  std::vector<Attrib> attribs;
};
};