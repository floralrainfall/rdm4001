#pragma once
#include <glm/glm.hpp>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace rdm::gfx {
enum DataType {
  DtUnsignedByte,
  DtByte,
  DtUnsignedShort,
  DtShort,
  DtUnsignedInt,
  DtInt,
  DtFloat,
  DtMat2,
  DtMat3,
  DtMat4,
  DtVec2,
  DtVec3,
  DtVec4,

  DtSampler,  // only useful for programs
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
    D8,
    D24S8,
  };

  enum Format {
    RGB,
    RGBA,
  };

  virtual ~BaseTexture() {};

  // virtual void upload1d(int width, DataType type, Format format, void* data,
  // int mipmapLevels = 0) = 0;

  virtual void reserve2d(int width, int height, InternalFormat format,
                         int mipmapLevels = 1) = 0;
  virtual void upload2d(int width, int height, DataType type, Format format,
                        void* data, int mipmapLevels = 0) = 0;

  /**
   * @brief Recreates the underlying texture. Reuploads are necessary
   *
   */
  virtual void destroyAndCreate() = 0;

  virtual void bind() = 0;

  Type getType() { return textureType; }
  InternalFormat getInternalFormat() { return textureFormat; }

  bool isReserve() { return reserve; };

 protected:
  Type textureType;
  InternalFormat textureFormat;

  /**
   * @brief Set this to TRUE if called using reserveX functions
   *
   */
  bool reserve;
};

struct ShaderFile {
  std::string code;
  std::string name;
};

/**
 * @brief A program shader.
 *
 * This will compile input shaders, and link them into a program.
 * If you don't know what you are doing, use a Material
 * (MaterialCache::getOrLoad)
 *
 */
class BaseProgram {
 public:
  enum Shader { Vertex, Fragment, Geometry };

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
    glm::mat2 matrix2x2;
    glm::mat3 matrix3x3;
    glm::mat4 matrix4x4;
    glm::vec2 vec2;
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

/**
 * @brief Buffer. Use BaseTexture to store texture data.
 *
 */
class BaseBuffer {
 public:
  /**
   * @brief Type of the buffer.
   *
   */
  enum Type {
    Unknown,
    /**
     * @brief An Element buffer. On OpenGL this is the buffer where you should
     * write indices. on OpenGL, this will call glDrawElements if drawn using
     * BaseDevice::draw
     */
    Element,
    /**
     * @brief An Array buffer. On OpenGL this is the buffer where you should
     * write vertex data. On OpenGL, this will call glDrawArrays if drawn using
     * BaseDevice::draw
     */
    Array
  };

  /**
   * @brief The usage type of a BaseBuffer.
   *
   */
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

  /**
   * @brief Uploads data to the buffer.
   *
   * @param type Buffer type. On GL this changes where the buffer is bound to.
   * This also changes how draw calls are executed, on GL.
   * @param usage The usage type.
   * @param size
   * @param data
   */
  virtual void upload(Type type, Usage usage, size_t size,
                      const void* data) = 0;
  virtual void bind() = 0;

 protected:
  std::mutex lock;
};

/**
 * @brief Array Pointers are analagous to VAO's in OpenGL.
 * This will probably need updating when I port gfx to other graphics libraries
 */
class BaseArrayPointers {
 public:
  struct Attrib {
    int layoutId;
    int size;  // 1, 2, 3, or 4
    DataType type;
    bool normalized;
    size_t stride;
    void* offset;
    BaseBuffer* buffer;  // optional external buffer

    Attrib(DataType type, int id, int size, size_t stride, void* offset,
           BaseBuffer* buffer = 0, bool normalized = false) {
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

  /**
   * @brief Binds the array pointers to the context.
   *
   */
  virtual void bind() = 0;

 protected:
  std::vector<Attrib> attribs;
};

/**
 * @brief A framebuffer.
 *
 * This allows to render commands, and have them be written to a texture.
 * There are no bind functions in this type, use the BaseDevice to bind
 * framebuffers to the rendering device.
 *
 * This requires you create a texture using BaseDevice::createTexture, and
 * reserve it using functions like BaseTexture::reserve2d.
 */
class BaseFrameBuffer {
  friend class Engine;

 public:
  enum Status {
    /**
     * @brief Missing attachments, add one with setTarget
     *
     */
    MissingAttachment,
    /**
     * @brief Dimensions are not the same across all targets
     *
     */
    BadDimensions,
    /**
     * @brief Attachment no longer exists, or there is another problem with the
     * attachment
     *
     * On OpenGL, problems include:
     *  - attachment point missing
     *  - attached image has a width or height of zero
     *  - color attachment has a non color-renderable image attached
     *  - depth attachment has a non depth-renderable image attached
     *  - stencil attachment has a non stencil-renderable image attached
     */
    BadAttachment,
    /**
     * @brief Framebuffer is ready to be rendered into.
     *
     */
    Complete,
    /**
     * @brief Framebuffers are unsupported by the device.
     *
     */
    Unsupported,
  };

  /**
   * @brief This controls where an input image will be attached to.
   *
   */
  enum AttachmentPoint {
    Color0,
    Color1,
    Color2,
    Color3,
    Depth,
    Stencil,
    DepthStencil
  };

  virtual ~BaseFrameBuffer() {};

  /**
   * @brief Sets the target texture of this framebuffer.
   *
   * @param texture The target texture. Make sure this lives for as long or
   * longer then this BaseFrameBuffer. I don't know how GLFrameBuffer will react
   * to that.
   */
  virtual void setTarget(BaseTexture* texture,
                         AttachmentPoint point = Color0) = 0;
  virtual Status getStatus() = 0;
  virtual void destroyAndCreate() = 0;
};
};  // namespace rdm::gfx