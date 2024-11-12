#pragma once
#include "worldspawn.hpp"

#include "map.hpp"
#include "network/bitstream.hpp"
#include "world.hpp"
namespace ww {
Worldspawn::Worldspawn(net::NetworkManager* manager, net::EntityId id)
    : net::Entity(manager, id) {
  file = NULL;
  entity = NULL;
  if (!getManager()->isBackend()) {
    worldJob = getWorld()->stepped.listen([this] {
      std::scoped_lock lock(mutex);
      if (file) file->updatePosition(getGfxEngine()->getCamera().getPosition());
    });
  }
  mapName.set("");
}

Worldspawn::~Worldspawn() {
  if (!getManager()->isBackend()) {
    if (entity) getGfxEngine()->deleteEntity(entity);
    getWorld()->stepped.removeListener(worldJob);
  }
}

void Worldspawn::addToEngine(rdm::gfx::Engine* engine) {
  std::scoped_lock lock(mutex);
  file->initGfx(engine);
  entity = engine->addEntity<MapEntity>(file);
  pendingAddToGfx = false;
}

void Worldspawn::addToWorld(rdm::World* world) {
  file->addToPhysicsWorld(world->getPhysicsWorld());
}

void Worldspawn::loadFile(const char* _file) {
  std::scoped_lock lock(mutex);
  file = new BSPFile(_file);
  mapName.set(_file);

  addToWorld(getWorld());
}

void Worldspawn::serialize(net::BitStream& stream) {
  mapName.serialize(stream);
}

void Worldspawn::deserialize(net::BitStream& stream) {
  std::scoped_lock lock(mutex);

  mapName.deserialize(stream);
  if (!file && mapName.get() != "") {
    file = new BSPFile(mapName.get().c_str());
    addToWorld(getWorld());
    pendingAddToGfx = true;
  }
}
}  // namespace ww
