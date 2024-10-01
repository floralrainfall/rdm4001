#include "heightmap.hpp"
namespace rdm::gfx {
void HeightmapEntity::renderTechnique(BaseDevice* device, int id) {
  arrayPointers->bind();
  device->draw(elementBuffer.get(), DtUnsignedByte, BaseDevice::Triangles,
               elementCount);
}

HeightmapEntity::HeightmapEntity(Graph::Node* node) : Entity(node) {}

void HeightmapEntity::generateMesh(Engine* engine) {
  setMaterial(
      engine->getMaterialCache()->getOrLoad("HeightmapTesselated").value());

  auto hmap =
      engine->getTextureCache()->getOrLoad2d("dat5/hmap1.png", true).value();
  std::vector<unsigned int> indices;

  std::vector<float> vertices;
  float yScale = 64.0f / 256.0f,
        yShift = 16.0f;  // apply a scale+shift to the height data
  for (unsigned int i = 0; i < hmap.first.height; i++) {
    for (unsigned int j = 0; j < hmap.first.width; j++) {
      // retrieve texel for (i,j) tex coord
      unsigned char* texel =
          hmap.first.data + (j + hmap.first.width * i) * hmap.first.channels;
      // raw height at coordinate
      unsigned char y = texel[0];

      // vertex
      vertices.push_back(-hmap.first.height / 2.0f + i);  // v.x
      vertices.push_back((int)y * yScale - yShift);       // v.y
      vertices.push_back(-hmap.first.width / 2.0f + j);   // v.z
    }
  }

  for (unsigned int i = 0; i < hmap.first.height - 1;
       i++)  // for each row a.k.a. each strip
  {
    for (unsigned int j = 0; j < hmap.first.width; j++)  // for each column
    {
      for (unsigned int k = 0; k < 2; k++)  // for each side of the strip
      {
        indices.push_back(j + hmap.first.width * (i + k));
      }
    }
  }
  elementCount = indices.size();

  elementBuffer = engine->getDevice()->createBuffer();
  elementBuffer->upload(BaseBuffer::Element, BaseBuffer::StaticDraw,
                        indices.size() * sizeof(unsigned int), indices.data());
  arrayBuffer = engine->getDevice()->createBuffer();
  arrayBuffer->upload(BaseBuffer::Array, BaseBuffer::StaticDraw,
                      vertices.size() * sizeof(float), vertices.data());

  arrayPointers = engine->getDevice()->createArrayPointers();
  arrayPointers->addAttrib(
      BaseArrayPointers::Attrib(DtFloat, 0, 3, 0, 0, arrayBuffer.get()));
}
}  // namespace rdm::gfx