#pragma once
#include "map.hpp"
#include "network/bitstream.hpp"
#include "network/entity.hpp"
namespace net = rdm::network;
namespace ww {
class Worldspawn : public net::Entity {
  std::unique_ptr<BSPFile> file;
  bool pendingAddToGfx;

 public:
  net::ReplicateProperty<std::string> mapName;

  Worldspawn(net::NetworkManager* manager, net::EntityId id);

  virtual const char* getTypeName() { return "Worldspawn"; };

  bool isPendingAddToGfx() { return pendingAddToGfx; }
  void addToEngine(rdm::gfx::Engine* engine);
  void addToWorld(rdm::World* world);
  void loadFile(const char* file);  // call on backend

  BSPFile* getFile() { return file.get(); }

  virtual void serialize(net::BitStream& stream);
  virtual void deserialize(net::BitStream& stream);
};
}  // namespace ww
