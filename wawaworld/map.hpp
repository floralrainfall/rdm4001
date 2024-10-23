#pragma once
#include <memory>

#include "gfx/base_types.hpp"
#include "gfx/engine.hpp"
#include "physics.hpp"

namespace ww {
using namespace rdm;
const int BSP_VERSION = 0x2e;

enum BSPEntry {
  BSP_ENTITIES,
  BSP_TEXTURES,
  BSP_PLANES,
  BSP_NODES,
  BSP_LEAFS,
  BSP_LEAFFACES,
  BSP_LEAFBRUSHES,
  BSP_MODELS,
  BSP_BRUSHES,
  BSP_BRUSHSIDES,
  BSP_VERTICES,
  BSP_MESHVERTS,
  BSP_EFFECTS,
  BSP_FACES,
  BSP_LIGHTMAPS,
  BSP_LIGHTVOLS,
  BSP_VISDATA,
  __BSP_DIRENT_MAX,
};

struct BSPDirentry {
  int offset;
  int length;

  void* loadData(std::vector<unsigned char>& data);
};

struct BSPFaceModel {
  gfx::BaseTexture* m_texture;
  gfx::BaseTexture* m_lightmap;
  std::unique_ptr<gfx::BaseBuffer> m_buffer;
  std::unique_ptr<gfx::BaseBuffer> m_index;
  std::shared_ptr<gfx::Material> m_program;
  int m_indexCount;
  std::unique_ptr<gfx::BaseArrayPointers> m_layout;
};

struct BSPHeader {
  char magic[4];
  int version;
  BSPDirentry dirents[__BSP_DIRENT_MAX];
};

// lump types

struct BSPTexture {
  char name[64];
  unsigned int flags;
  unsigned int contents;
};

struct BSPVertex {
  glm::vec3 position;
  glm::vec2 surface_uv;
  glm::vec2 lm_uv;
  glm::vec3 normal;
  unsigned char color[4];
};

struct BSPLightmap {
  unsigned char lightmap[128][128][3];
};

struct BSPFace {
  unsigned int texture;
  unsigned int effect;
  unsigned int type;
  unsigned int vertex;
  unsigned int n_vertices;
  unsigned int meshvert;
  unsigned int n_meshverts;
  int lm_index;
  glm::ivec2 lm_start;
  glm::ivec2 lm_size;
  glm::vec3 lm_origin;
  glm::vec3 lm_vecs[2];
  glm::vec3 normal;
  glm::ivec2 size;
};

struct BSPNode {
  int plane;
  int children[2];
  int maxs[3];
  int mins[3];
};

struct BSPLeaf {
  unsigned int cluster;
  unsigned int area;
  int mins[3];
  int maxs[3];
  unsigned int leafface;
  unsigned int n_leaffaces;
  unsigned int leafbrush;
  unsigned int n_leafbrushes;
};

struct BSPVisdata {
  unsigned int n_vecs;
  unsigned int sz_vecs;
  unsigned char data[];
};

struct BSPPlane {
  glm::vec3 normal;
  float dist;
};

struct BSPBrush {
  unsigned int brushside;
  unsigned int n_brushsides;
  unsigned int texture;
};

struct BSPBrushSide {
  unsigned int plane;
  unsigned int texture;
};

struct BSPBrushModelSide {
  BSPPlane plane;
};
  
struct BSPBrushModel {
  std::vector<BSPBrushModelSide> brushSides;
  unsigned int texture;
};

class BSPFile {
  struct BSPLeafModel {
    std::vector<BSPFaceModel> m_models;
    int m_cluster;
    int mins[3];
    int maxs[3];
  };
  void* direntData[__BSP_DIRENT_MAX];
  bool m_useVis;
  bool m_gfxEnabled;
  std::string m_name;
  PhysicsWorld* m_physicsWorld;

  BSPHeader m_header;
  std::vector<BSPLeafModel> m_leafs;
  std::vector<BSPFaceModel> m_models;
  std::vector<BSPBrushModel> m_brushes;
  std::vector<std::unique_ptr<gfx::BaseTexture>> m_textures;
  std::vector<btRigidBody*> m_brushBodies;
  glm::vec3 vecpos;
  int m_currentClusterIndex;
  int skyboxCluster;
  int m_facesRendered;
  int m_leafsRendered;

  gfx::BaseTexture* m_skybox;

  void readEntitesLump(BSPDirentry* dirent);
  void addLeafFaces(BSPLeaf* leaf, bool brush, bool leafface);
  void parseTreeNode(BSPNode* node, bool brush, bool leafface);
  BSPFaceModel addFaceModel(BSPFace* face);
  void renderFaceModel(BSPFaceModel* model, gfx::BaseProgram* program);

  gfx::Engine* engine;
  
 public:
  BSPFile(const char* bsp);
  ~BSPFile();

  std::unique_ptr<gfx::BaseArrayPointers> createModelLayout();
  void addToPhysicsWorld(PhysicsWorld* world);
  void removeFromPhysicsWorld(PhysicsWorld* world);
  bool getUsingVis() { return m_useVis; }
  bool getGfxEnabled() { return m_gfxEnabled; }
  int getVisCluster() { return m_currentClusterIndex; }
  int getFacesRendered() { return m_facesRendered; }
  int getFacesTotal() { return m_models.size(); }
  int getLeafsRendered() { return m_leafsRendered; }
  int getLeafsTotal() { return m_leafs.size(); }
  //  void hwapiDraw(HWProgramReference* program);
  void updatePosition(glm::vec3 new_pos);
  void setSkybox(gfx::BaseTexture* skybox);
  int getCluster(glm::vec3 p);
  bool canSeeCluster(int x, int y);
  bool canSee(glm::vec3 x, glm::vec3 y);
  std::string getName() { return m_name; }

  void draw();

  void initGfx(gfx::Engine* engine);
};

class MapEntity : public gfx::Entity {
  virtual void renderTechnique(gfx::BaseDevice* device, int id);
  BSPFile* f;
 public:
  MapEntity(BSPFile* rsc, Graph::Node* node = NULL);
  virtual ~MapEntity();

  virtual void initialize();
};
};  // namespace ww
