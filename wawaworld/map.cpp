#include "map.hpp"

#include <bullet/Bullet3Geometry/b3GeometryUtil.h>

#include <climits>
#include <sstream>
#include <stdexcept>

#include "filesystem.hpp"
#include "fun.hpp"
#include "gfx/base_device.hpp"
#include "gfx/base_types.hpp"
#include "gfx/camera.hpp"
#include "gfx/engine.hpp"
#include "gfx/entity.hpp"
#include "gfx/rendercommand.hpp"
#include "gfx/renderpass.hpp"
#include "logging.hpp"
#include "physics.hpp"
#include "wgame.hpp"

#ifndef DISABLE_EASY_PROFILER
#include <easy/profiler.h>
#endif

// taken from matrix

namespace ww {
void* BSPDirentry::loadData(std::vector<unsigned char>& data) {
  void* d = malloc(length);
  memcpy(d, data.data() + offset, length);
  // size_t fpos = std::ftell(bsp);
  // std::fseek(bsp, offset, SEEK_SET);
  // void* d = malloc(length);
  // std::fread(d, 1, length, bsp);
  // std::fseek(bsp, fpos, SEEK_SET);
  return d;
}

BSPFile::BSPFile(const char* bsp) {
  m_name = bsp;
  m_gfxEnabled = false;
  common::OptionalData od = common::FileSystem::singleton()->readFile(bsp);
  m_currentClusterIndex = 0;
  m_useVis = true;
  m_skybox = NULL;
  // m_physicsWorld = NULL;
  if (od) {
    memcpy(&m_header, od.value().data(), sizeof(BSPHeader));

    // preload all dirent entries
    for (int i = 0; i < __BSP_DIRENT_MAX; i++) {
      BSPDirentry* ent = &m_header.dirents[i];
      void* ent_data = ent->loadData(od.value());
      direntData[i] = ent_data;
      Log::printf(LOG_DEBUG, "Loaded dirent %i (size %i bytes)", i,
                  ent->length);
    }

    BSPDirentry* f = &m_header.dirents[BSP_ENTITIES];
    readEntitesLump(f);

    BSPNode* root = (BSPNode*)direntData[BSP_NODES];
    parseTreeNode(root, true, false);
  } else {
    throw std::runtime_error("Could not load bsp file");
  }
}

BSPFile::~BSPFile() {
  // if (m_physicsWorld) removeFromPhysicsWorld(m_physicsWorld);
  if (m_gfxEnabled) {
    for (BSPFaceModel& model : m_models) {
    }
  }
}

void BSPFile::readEntitesLump(BSPDirentry* dirent) {
  std::string entityData((char*)direntData[BSP_ENTITIES], dirent->length);
  std::stringstream ss(entityData);
  std::string str;

  bool inClass;
  std::map<std::string, std::string> classProperties;

  while (std::getline(ss, str, '\n')) {
    if (str == "{") {
      classProperties.clear();
      inClass = true;
    } else if (str == "}") {
      BSPEntity e;
      e.properties = classProperties;
      entities.push_back(e);
      inClass = false;
    } else {
      if (!inClass) {
        Log::printf(LOG_WARN, "Value placed out of class definition");
        continue;
      }
      if (str.empty()) continue;
      size_t p = str.find(' ');
      if (p == std::string::npos) {
        Log::printf(LOG_WARN, "p = std::string::npos (%s)", str.c_str());
        continue;
      }
      std::string n = str.substr(0, p);
      std::string v = str.substr(p + 1);
      n = n.substr(1, n.size() - 2);
      v = v.substr(1, v.size() - 2);
      classProperties[n] = v;
    }
  }
}

void BSPFile::addLeafFaces(BSPLeaf* leaf, bool brushen, bool leaffaceen) {
  int leafcount = m_header.dirents[BSP_LEAFS].length / sizeof(BSPLeaf);
  int brushcount = m_header.dirents[BSP_BRUSHES].length / sizeof(BSPBrush);

  // DEV_MSG("leaf %p cluster %i area %i n_leaffaces %i", leaf, leaf->cluster,
  // leaf->area, leaf->n_leaffaces);
  // Log::printf(LOG_DEBUG, "Leaf %p Cluster %i Area %i Leaffaces %i", leaf,
  //            leaf->cluster, leaf->area, leaf->n_leaffaces);
  int* leaffaces = (int*)direntData[BSP_LEAFFACES];
  int* leafbrushes = (int*)direntData[BSP_LEAFBRUSHES];
  BSPFace* faces = (BSPFace*)direntData[BSP_FACES];
  BSPBrush* brushes = (BSPBrush*)direntData[BSP_BRUSHES];
  BSPBrushSide* brushsides = (BSPBrushSide*)direntData[BSP_BRUSHSIDES];
  BSPPlane* planes = (BSPPlane*)direntData[BSP_PLANES];
  BSPLeafModel m;
  BSPVisdata* visdata = (BSPVisdata*)direntData[BSP_VISDATA];

  m.m_cluster = leaf->cluster;

  for (int b = leaf->leafbrush; b < (leaf->leafbrush + leaf->n_leafbrushes);
       b++) {
    if (b >
        m_header.dirents[BSP_LEAFBRUSHES].length / sizeof(BSP_LEAFBRUSHES)) {
      Log::printf(LOG_ERROR, "BSP error: loaded bad leaf brush %i", b);
      return;
    }
    if (!brushen) break;
    int brushid = leafbrushes[b];
    if (brushid > brushcount || brushid < 0) break;
    BSPBrush* brush = &brushes[brushid];
    BSPBrushModel brushmodel;
    for (int s = brush->brushside; s < (brush->brushside + brush->n_brushsides);
         s++) {
      BSPBrushModelSide bmside;
      BSPBrushSide* side = &brushsides[s];
      BSPPlane* plane = &planes[side->plane];
      bmside.plane = *plane;
      brushmodel.brushSides.push_back(bmside);
    }
    m_brushes.push_back(brushmodel);
  }

  if (leaf->cluster > visdata->n_vecs) return;

  if (m.m_cluster < 0) {
    Log::printf(LOG_INFO, "Cluster is out of world or invalid");
    return;
  } else {
    if (m_gfxEnabled && leaffaceen) {
      for (int f = leaf->leafface; f < (leaf->leafface + leaf->n_leaffaces);
           f++) {
        if (f >
            m_header.dirents[BSP_LEAFFACES].length / sizeof(BSP_LEAFFACES)) {
          Log::printf(LOG_ERROR, "BSP error: loaded bad leaf face %i", f);
          return;
        }

        BSPFace* face = &faces[leaffaces[f]];
        m.m_models.push_back(addFaceModel(face));
      }
    }
  }

  std::memcpy(m.mins, leaf->mins, sizeof(m.mins) + sizeof(m.maxs));

  m_leafs.push_back(std::move(m));
}

BSPFaceModel BSPFile::addFaceModel(BSPFace* face) {
  BSPFaceModel m;
  m.m_buffer = engine->getDevice()->createBuffer();
  m.m_index = engine->getDevice()->createBuffer();
  m.m_layout = engine->getDevice()->createArrayPointers();
  std::vector<BSPVertex> vertices;
  std::vector<int> indices;

  int* meshverts = (int*)direntData[BSP_MESHVERTS];
  BSPVertex* verts = (BSPVertex*)direntData[BSP_VERTICES];
  BSPLightmap* lightmaps = (BSPLightmap*)direntData[BSP_LIGHTMAPS];

  switch (face->type) {
    default:
      Log::printf(LOG_WARN, "unknown face type in bsp %i", face->type);
      break;
    case 1:  // type 1, polygon
    case 3:  // type 3, mesh vertices
      for (int v = face->meshvert; v < (face->meshvert + face->n_meshverts);
           v++)
        indices.push_back(meshverts[v]);
      for (int v = face->vertex; v < (face->vertex + face->n_vertices); v++)
        vertices.push_back(verts[v]);
      std::reverse(indices.begin(),
                   indices.end());  // the indices must be flipped so it renders
                                    // the inside of the mesh
      m.m_layout->addAttrib(gfx::BaseArrayPointers::Attrib(
          gfx::DataType::DtFloat, 0, 3, sizeof(BSPVertex),
          (void*)offsetof(BSPVertex, position), m.m_buffer.get()));
      m.m_layout->addAttrib(gfx::BaseArrayPointers::Attrib(
          gfx::DataType::DtFloat, 1, 3, sizeof(BSPVertex),
          (void*)offsetof(BSPVertex, normal), m.m_buffer.get()));
      m.m_layout->addAttrib(gfx::BaseArrayPointers::Attrib(
          gfx::DataType::DtFloat, 2, 2, sizeof(BSPVertex),
          (void*)offsetof(BSPVertex, surface_uv), m.m_buffer.get()));
      m.m_layout->addAttrib(gfx::BaseArrayPointers::Attrib(
          gfx::DataType::DtFloat, 3, 2, sizeof(BSPVertex),
          (void*)offsetof(BSPVertex, lm_uv), m.m_buffer.get()));
      m.m_layout->addAttrib(gfx::BaseArrayPointers::Attrib(
          gfx::DataType::DtFloat, 4, 4, sizeof(BSPVertex),
          (void*)offsetof(BSPVertex, color), m.m_buffer.get()));
      break;
  }

  m.m_lightmap = 0;
  if (face->lm_index >= 0) {
    BSPLightmap* lm = &lightmaps[abs(face->lm_index)];
    char tname[64];
    memset(tname, 0, 64);
    snprintf(tname, 64, "lm:%s%i", m_name.c_str(), face->lm_index);
    auto lmt = engine->getTextureCache()->get(tname);
    if (!lmt) {
      Log::printf(LOG_DEBUG, "loading lightmap %s", tname);
      std::unique_ptr<gfx::BaseTexture> _lmt =
          engine->getDevice()->createTexture();
      _lmt->upload2d(128, 128, gfx::DataType::DtUnsignedByte,
                     gfx::BaseTexture::RGB, lm->lightmap);
      lmt = std::pair<gfx::TextureCache::Info, gfx::BaseTexture*>(
          gfx::TextureCache::Info{},
          engine->getTextureCache()->cacheExistingTexture(
              tname, _lmt, gfx::TextureCache::Info{}));
    }
    m.m_lightmap = lmt->second;
  }

  // first part is designed to fit with ModelComponent layout so i can reuse
  // materials

  m.m_buffer->upload(gfx::BaseBuffer::Array, gfx::BaseBuffer::StaticDraw,
                     vertices.size() * sizeof(BSPVertex), vertices.data());
  m.m_indexCount = indices.size() * sizeof(int);
  m.m_index->upload(gfx::BaseBuffer::Element, gfx::BaseBuffer::StaticDraw,
                    m.m_indexCount, indices.data());

  BSPTexture texture = ((BSPTexture*)direntData[BSP_TEXTURES])[face->texture];

  try {
    if (texture.name ==
        std::string("textures/skies/skybox"))  // TODO: make this not hardcoded
    {
      m.type = BSPFaceModel::Sky;
    } else {
      m.type = BSPFaceModel::Opaque;
      std::string extensions[] = {
          ".png", ".jpg", ".tga", ".PNG", ".JPG", ".TGA",
      };
      m.m_texture = 0;
      for (std::string extension : extensions) {
        std::string txpath =
            std::string("dat5/baseq3/") + texture.name + extension;
        auto t = engine->getTextureCache()->getOrLoad2d(txpath.c_str());
        if (t) {
          m.m_texture = t->second;
          break;
        }
      }
      if (m.m_texture == 0) {
        Log::printf(LOG_WARN, "Could not find texture %s", texture.name);
        auto t =
            engine->getTextureCache()->getOrLoad2d("dat5/missingtexture.png");
        if (t)
          m.m_texture = t->second;
        else
          m.m_texture = 0;
      }
      if (m.m_texture) {
        m.m_texture->setFiltering(gfx::BaseTexture::Nearest,
                                  gfx::BaseTexture::Nearest);
      }
    }
  } catch (std::exception& e) {
    m.m_texture = 0;
  }

  m.m_layout->upload();
  // m_models.push_back(std::move(m));

  return m;
}

void BSPFile::updatePosition(glm::vec3 new_pos) {
  int oldCluster = m_currentClusterIndex;
  m_currentClusterIndex = getCluster(new_pos);
  if (oldCluster != m_currentClusterIndex) {
    Log::printf(LOG_DEBUG, "new cluster %i", m_currentClusterIndex);
  }
}

void BSPFile::parseTreeNode(BSPNode* node, bool brush, bool leafface) {
  // Log::printf(LOG_DEBUG, "node %p, a: %i, b: %i", node, node->children[0],
  //	      node->children[1]);
  BSPLeaf* leafs = (BSPLeaf*)direntData[BSP_LEAFS];
  BSPNode* nodes = (BSPNode*)direntData[BSP_NODES];
  int leafcount = m_header.dirents[BSP_LEAFS].length / sizeof(BSPLeaf);

  if (node->children[0] < 0) {
    int leaf = -(node->children[0]);
    BSPLeaf* leafNode = &leafs[leaf];
    if (leaf < leafcount) addLeafFaces(leafNode, brush, leafface);
  } else {
    BSPNode* cnode = &nodes[node->children[0]];
    parseTreeNode(cnode, brush, leafface);
  }

  if (node->children[1] < 0) {
    int leaf = -(node->children[1]);
    BSPLeaf* leafNode = &leafs[leaf];
    if (leaf < leafcount) addLeafFaces(leafNode, brush, leafface);
  } else {
    BSPNode* cnode = &nodes[node->children[1]];
    parseTreeNode(cnode, brush, leafface);
  }
}

void BSPFile::renderFaceModel(gfx::RenderList& list, BSPFaceModel* model,
                              gfx::BaseProgram* program) {
  if (!m_gfxEnabled) return;
  if (!model->m_index.get()) return;

  /*model->m_layout->bind();
  program->setParameter(
      "lightmap", gfx::DtSampler,
      gfx::BaseProgram::Parameter{.texture.texture = model->m_lightmap,
                                  .texture.slot = 2});
  program->setParameter(
      "surface", gfx::DtSampler,
      gfx::BaseProgram::Parameter{.texture.texture = model->m_texture,
                                  .texture.slot = 0});

  program->setParameter(
      "skybox", gfx::DtSampler,
      gfx::BaseProgram::Parameter{.texture.texture = m_skybox.get(),
      .texture.slot = 1});*/

  gfx::RenderCommand command(gfx::BaseDevice::Triangles, model->m_index.get(),
                             model->m_indexCount / sizeof(int),
                             model->m_layout.get());
  if (model->type == BSPFaceModel::Opaque) {
    command.setTexture(0, model->m_texture);
    command.setTexture(1, model->m_lightmap);
  }
  list.add(command);
  /*engine->getDevice()->draw(model->m_index.get(), gfx::DtUnsignedInt,
                            gfx::BaseDevice::Triangles,
                            model->m_indexCount / sizeof(int));*/
}

void BSPFile::draw() {
  if (!m_gfxEnabled) return;
#ifndef DISABLE_EASY_PROFILER
  EASY_FUNCTION("BSPFile::draw");
#endif

  gfx::RenderListSettings settings;
  settings.cull = rdm::gfx::BaseDevice::BackCCW;

  gfx::RenderList opaque(engine->getMaterialCache()
                             ->getOrLoad("BspBrush")
                             .value()
                             ->prepareDevice(engine->getDevice(), 0),
                         NULL, settings);

  gfx::BaseProgram* sky =
      engine->getMaterialCache()->getOrLoad("BspSky").value()->prepareDevice(
          engine->getDevice(), 0);
  sky->setParameter("skybox", gfx::DtSampler,
                    gfx::BaseProgram::Parameter{.texture.slot = 0,
                                                .texture.texture = m_skybox});
  gfx::RenderListSettings skySettings;
  skySettings.cull = rdm::gfx::BaseDevice::BackCCW;
  skySettings.state = rdm::gfx::BaseDevice::Disabled;
  gfx::RenderList skybox(sky, NULL, skySettings);

  gfx::RenderList transparent(engine->getMaterialCache()
                                  ->getOrLoad("BspBrush")
                                  .value()
                                  ->prepareDevice(engine->getDevice(), 0),
                              NULL, settings);

  m_facesRendered = 0;
  m_leafsRendered = 0;
  gfx::Frustrum frustrum = engine->getCamera().computeFrustrum();

  if (true) {  // TODO: use Settings or bring back matrix style ConVar system
    for (int i = 0; i < m_leafs.size(); i++) {
      BSPLeafModel& leaf = m_leafs.at(i);

      if (!leaf.m_models.size()) continue;

      int y = m_currentClusterIndex;
      int x = leaf.m_cluster;

      if (!canSeeCluster(x, y)) continue;

      gfx::Frustrum::TestResult result =
          frustrum.test(glm::vec3(leaf.mins[0], leaf.mins[1], leaf.mins[2]),
                        glm::vec3(leaf.maxs[0], leaf.maxs[1], leaf.maxs[2]));
      if (result == gfx::Frustrum::Outside) continue;

      for (int j = 0; j < leaf.m_models.size(); j++) {
        BSPFaceModel& model = leaf.m_models[j];
        switch (model.type) {
          case BSPFaceModel::Opaque:
            renderFaceModel(opaque, &model, NULL);
            break;
          case BSPFaceModel::Sky:
            renderFaceModel(skybox, &model, NULL);
            break;
          case BSPFaceModel::Transparent:
            renderFaceModel(transparent, &model, NULL);
            break;
        }
        m_facesRendered++;
      }
      m_leafsRendered++;
    }
  } else {
    for (int i = 0; i < m_models.size(); i++) {
      BSPFaceModel& model = m_models.at(i);
      switch (model.type) {
        case BSPFaceModel::Opaque:
          renderFaceModel(opaque, &model, NULL);
          break;
        case BSPFaceModel::Sky:
          renderFaceModel(skybox, &model, NULL);
          break;
        default:
          break;
      }
      m_facesRendered++;
    }
  }

  opaque.sort([](gfx::RenderCommand const& a, gfx::RenderCommand const& b) {
    gfx::BaseTexture *_a, *_b;
    _a = a.getTexture(1);
    _b = b.getTexture(1);

    return _a == _b ? (a.getTexture(0) < b.getTexture(0)) : (_a < _b);
  });

  engine->pass(gfx::RenderPass::Opaque).add(skybox);
  engine->pass(gfx::RenderPass::Opaque).add(opaque);
  engine->pass(gfx::RenderPass::Transparent).add(transparent);
}

void BSPFile::removeFromPhysicsWorld(PhysicsWorld* world) {
  for (btRigidBody* body : m_brushBodies) {
    delete body->getMotionState();
    delete body->getCollisionShape();
    world->getWorld()->removeRigidBody(body);
    delete body;
  }
  m_brushBodies.clear();
}

void BSPFile::addToPhysicsWorld(PhysicsWorld* world) {
  std::scoped_lock lock(world->mutex);

  int rb_added = 0;
  for (auto brush : m_brushes) {
    b3AlignedObjectArray<b3Vector3> planeeqs;

    for (auto brushside : brush.brushSides) {
      b3Vector3 planeEq;
      planeEq.setValue(brushside.plane.normal[0], brushside.plane.normal[1],
                       brushside.plane.normal[2]);
      planeEq[3] = -brushside.plane.dist;
      planeeqs.push_back(planeEq);
    }

    b3AlignedObjectArray<b3Vector3> verts;
    b3GeometryUtil::getVerticesFromPlaneEquations(planeeqs, verts);
    btVector3 etg;

    // i dont know why i must do this but i have to
    std::vector<btVector3> intermediate;
    for (int i = 0; i < verts.size(); i++) {
      b3Vector3 vert = verts.at(i);
      intermediate.push_back(btVector3(vert.x, vert.y, vert.z));
    }

    if (intermediate.size() != 0) {
      btCollisionShape* brushshape = new btConvexHullShape(
          (btScalar*)intermediate.data(), intermediate.size());
      btRigidBody* brushbody = new btRigidBody(0.0, NULL, brushshape);
      brushbody->setUserPointer(this);
      brushbody->setUserIndex(PHYSICS_INDEX_WORLD);
      brushshape->setUserPointer(this);
      brushshape->setUserIndex(PHYSICS_INDEX_WORLD);
      world->getWorld()->addRigidBody(brushbody);
      m_brushBodies.push_back(brushbody);
      rb_added++;
    }
  }
  Log::printf(LOG_DEBUG, "added %i brush bodies", rb_added);
  m_physicsWorld = world;
}

int BSPFile::findLeaf(glm::vec3 position) {
  int i = 0;
  while (i >= 0) {
    BSPNode node = ((BSPNode*)direntData[BSP_NODES])[i];
    BSPPlane plane = ((BSPPlane*)direntData[BSP_PLANES])[node.plane];
    float distance = glm::dot(plane.normal, position) - plane.dist;
    i = distance >= 0 ? node.children[0] : node.children[1];
  }
  return -i - 1;
}

int BSPFile::getCluster(glm::vec3 p) {
  // glm::ivec3 posi =
  //     glm::ivec3((int)roundf(p.x), (int)roundf(p.y), (int)roundf(p.z));
  int leaf = findLeaf(p);
  return ((BSPLeaf*)direntData[BSP_LEAFS])[leaf].cluster;
  /*for (BSPLeafModel& model : m_leafs) {
    if (posi.x >= model.mins[0] && posi.x <= model.maxs[0] &&
        posi.y >= model.mins[1] && posi.y <= model.maxs[1] &&
        posi.z >= model.mins[2] && posi.z <= model.maxs[2]) {
      // inside this leaf
      return model.m_cluster;
    }
    }*/
  // return -1;
}

bool BSPFile::canSeeCluster(int x, int y) {
  if (x == -1 || y == -1) return true;

  BSPVisdata* vdata = (BSPVisdata*)direntData[BSP_VISDATA];
  int vidx = x * vdata->sz_vecs + (y / 8);
  if (vidx > m_header.dirents[BSP_VISDATA].length) return false;

  // return vdata->data[vidx] & (1 << y % 8);
  return vdata->data[vidx] & (1 << (y & 7));
}

bool BSPFile::canSee(glm::vec3 x, glm::vec3 y) {
  return canSeeCluster(getCluster(x), getCluster(y));
}

void BSPFile::initGfx(gfx::Engine* engine) {
  m_gfxEnabled = true;
  this->engine = engine;

  /*m_skybox = App::getHWAPI()->newTexture();
  char* cubemap_textures[] = {
      "textures/skybox/gen0/positive_x.png",
      "textures/skybox/gen0/negative_x.png",
      "textures/skybox/gen0/positive_y.png",
      "textures/skybox/gen0/negative_y.png",
      "textures/skybox/gen0/negative_z.png",
      "textures/skybox/gen0/positive_z.png",
  };
  m_skybox->loadCubemap(cubemap_textures);*/

  auto _skybox = engine->getTextureCache()->get("null_plainsky512");
  if (_skybox) {
    m_skybox = _skybox.value().second;
  } else {
    try {
      std::vector<void*> cubemap_textures = {
          engine->getTextureCache()
              ->getOrLoad2d(
                  "dat5/baseq3/textures/skies/null_plainsky512_rt.jpg", true)
              .value()
              .first.data,
          engine->getTextureCache()
              ->getOrLoad2d(
                  "dat5/baseq3/textures/skies/null_plainsky512_lf.jpg", true)
              .value()
              .first.data,
          engine->getTextureCache()
              ->getOrLoad2d(
                  "dat5/baseq3/textures/skies/null_plainsky512_dn.jpg", true)
              .value()
              .first.data,
          engine->getTextureCache()
              ->getOrLoad2d(
                  "dat5/baseq3/textures/skies/null_plainsky512_up.jpg", true)
              .value()
              .first.data,
          engine->getTextureCache()
              ->getOrLoad2d(
                  "dat5/baseq3/textures/skies/null_plainsky512_bk.jpg", true)
              .value()
              .first.data,
          engine->getTextureCache()
              ->getOrLoad2d(
                  "dat5/baseq3/textures/skies/null_plainsky512_ft.jpg", true)
              .value()
              .first.data,
      };
      std::unique_ptr<gfx::BaseTexture> skybox =
          engine->getDevice()->createTexture();
      skybox->uploadCubeMap(
          engine->getTextureCache()
              ->getOrLoad2d(
                  "dat5/baseq3/textures/skies/null_plainsky512_lf.jpg")
              .value()
              .first.width,
          engine->getTextureCache()
              ->getOrLoad2d(
                  "dat5/baseq3/textures/skies/null_plainsky512_lf.jpg")
              .value()
              .first.height,
          cubemap_textures);
      gfx::TextureCache::Info info;
      info.channels = 3;
      info.data = NULL;
      info.width = 256;
      info.height = 256;
      m_skybox = skybox.get();
      engine->getTextureCache()->cacheExistingTexture("null_plainsky512",
                                                      skybox, info);
    } catch (std::exception& e) {
      Log::printf(LOG_ERROR, "error loading skybox, %s", e.what());
    }
  }

  BSPNode* root = (BSPNode*)direntData[BSP_NODES];

  Log::printf(LOG_DEBUG, "loading gfx: %i nodes, %i leafs, %i brushes",
              m_header.dirents[BSP_NODES].length / sizeof(BSPNode),
              m_header.dirents[BSP_LEAFS].length / sizeof(BSPLeaf),
              m_header.dirents[BSP_BRUSHES].length / sizeof(BSPBrush));
  Log::printf(LOG_DEBUG, "%i lightmaps, %i textures",
              m_header.dirents[BSP_LIGHTMAPS].length / sizeof(BSPLightmap),
              m_header.dirents[BSP_TEXTURES].length / sizeof(BSPTexture));

  parseTreeNode(root, false, true);
}

MapEntity::MapEntity(BSPFile* rsc, Graph::Node* node) : gfx::Entity(node) {
  f = rsc;
}

MapEntity::~MapEntity() {}

void MapEntity::initialize() {}

void MapEntity::renderTechnique(gfx::BaseDevice* device, int id) {
  if (!f) return;
  f->draw();
}
};  // namespace ww
