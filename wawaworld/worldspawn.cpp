#pragma once
#include "worldspawn.hpp"

#include "map.hpp"
#include "network/bitstream.hpp"
#include "world.hpp"
namespace ww {
Worldspawn::Worldspawn(net::NetworkManager* manager, net::EntityId id)
    : net::Entity(manager, id) {
  file = NULL;
}

void Worldspawn::addToEngine(rdm::gfx::Engine* engine) {
  file->initGfx(engine);
  engine->addEntity<MapEntity>(file.get());
  pendingAddToGfx = false;
}

void Worldspawn::addToWorld(rdm::World* world) {
  file->addToPhysicsWorld(world->getPhysicsWorld());
}

void Worldspawn::loadFile(const char* _file) {
  file.reset(new BSPFile(_file));
  mapName.set(_file);

  addToWorld(getWorld());
}

void Worldspawn::serialize(net::BitStream& stream) {
  mapName.serialize(stream);
}

void Worldspawn::deserialize(net::BitStream& stream) {
  if (file) {
    throw std::runtime_error("Cannot change map if map is already loaded");
  }

  mapName.deserialize(stream);
  file.reset(new BSPFile(mapName.get().c_str()));

  addToWorld(getWorld());
  pendingAddToGfx = true;
}
}  // namespace ww
