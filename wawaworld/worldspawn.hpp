#pragma once
#include "gfx/entity.hpp"
#include "map.hpp"
#include "network/bitstream.hpp"
#include "network/entity.hpp"
namespace net = rdm::network;
namespace ww {
class Worldspawn : public net::Entity {
  BSPFile*
      file;  // TODO: fix leak on ~Worldspawn, needs to happen on gfx thread
  rdm::ClosureId worldJob;
  bool pendingAddToGfx;
  rdm::gfx::Entity* entity;

  std::mutex mutex;

 public:
  net::ReplicateProperty<std::string> mapName;

  Worldspawn(net::NetworkManager* manager, net::EntityId id);
  virtual ~Worldspawn();

  virtual const char* getTypeName() { return "Worldspawn"; };

  bool isPendingAddToGfx() { return pendingAddToGfx; }
  void addToEngine(rdm::gfx::Engine* engine);
  void addToWorld(rdm::World* world);
  void loadFile(const char* file);  // call on backend

  BSPFile* getFile() { return file; }

  virtual void serialize(net::BitStream& stream);
  virtual void deserialize(net::BitStream& stream);
};
}  // namespace ww
