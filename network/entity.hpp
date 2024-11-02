#pragma once
#include <stdint.h>

#include <string>

#include "bitstream.hpp"
#include "signal.hpp"

namespace rdm {
class World;

namespace gfx {
class Engine;
}
}  // namespace rdm

namespace rdm::network {
typedef uint16_t EntityId;

enum ReplicateReliability {
  Reliable,
  ReliableSequencedUpdates = Reliable,
  ReliableUnsequencedUpdates,
  UnreliableSequencedUpdates,
  Unreliable,
};

template <typename T>
class ReplicateProperty {
  friend class NetworkManager;
  const char* type = typeid(T).name();
  T value;
  bool dirty;

  void setRemote(T v) {
    changingRemotely.fire(value, v);
    changing.fire(value, v);
    value = v;
    dirty = false;
  }

 public:
  // old, new
  Signal<T, T> changingLocally;
  Signal<T, T> changingRemotely;
  Signal<T, T> changing;

  void set(T v) {
    changingLocally.fire(value, v);
    changing.fire(value, v);
    value = v;
    dirty = true;
  }

  T get() { return value; }

  bool isDirty() { return dirty; };
  void clearDirty() { dirty = false; }

  void serialize(BitStream& stream);
  void deserialize(BitStream& stream);

  const char* getType() { return type; }
};

class NetworkManager;
class Entity {
  NetworkManager* manager;
  EntityId id;

 public:
  // use NetworkManager::instantiate
  Entity(NetworkManager* manager, EntityId id);
  virtual ~Entity();

  World* getWorld();
  NetworkManager* getManager() { return manager; }
  gfx::Engine* getGfxEngine();

  EntityId getEntityId() { return id; }

  virtual void tick() {};

  virtual void serialize(BitStream& stream) {};
  virtual void deserialize(BitStream& stream) {};

  virtual void serializeUnreliable(BitStream& stream) {};
  virtual void deserializeUnreliable(BitStream& stream) {};

  virtual bool dirty() { return false; }
  virtual const char* getTypeName() { return "Entity"; };
};

template <typename T>
Entity* EntityConstructor(NetworkManager* manager, EntityId id) {
  return new T(manager, id);
}
};  // namespace rdm::network