#include "entity.hpp"

#include <string>

namespace rdm::network {
template <>
void ReplicateProperty<std::string>::serialize(BitStream& stream) {
  stream.write<uint16_t>(value.size());
  for (int i = 0; i < value.size(); i++) stream.write<char>(value[i]);
}

template <>
void ReplicateProperty<std::string>::deserialize(BitStream& stream) {
  uint16_t size = stream.read<uint16_t>();
  std::string str;
  str.resize(size);
  for (int i = 0; i < size; i++) str[i] = stream.read<char>();
  setRemote(str);
}

template <>
void ReplicateProperty<int>::serialize(BitStream& stream) {
  stream.write(value);
}

template <>
void ReplicateProperty<int>::deserialize(BitStream& stream) {
  setRemote(stream.read<int>());
}

Entity::Entity(NetworkManager* manager, EntityId id) {
  this->id = id;
  this->manager = manager;
}

Entity::~Entity() {}
}  // namespace rdm::network
