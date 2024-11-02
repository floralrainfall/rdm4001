#include "bitstream.hpp"

#include <enet/enet.h>
#include <enet/types.h>
#include <stdlib.h>

#include <stdexcept>

#include "logging.hpp"

namespace rdm::network {
BitStream::BitStream() {
  data = 0;
  c = 0;
  size = 0;
}
BitStream::BitStream(BitStream& stream) {
  data = (char*)malloc(stream.size);
  c = 0;
  size = stream.size;
  memcpy(data, stream.data, size);
}
BitStream::BitStream(void* data, size_t size) {
  this->data = (char*)malloc(size);
  this->c = 0;
  this->size = size;
  memcpy(this->data, data, size);
}

BitStream::~BitStream() {
  if (data) free(data);
}

void BitStream::makeSpaceFor(size_t s) {
  if (size) {
    if (s + c > size) {
      size_t newSize = size * 2;
      size = std::max(s + c, newSize);
      data = (char*)realloc(data, size);
    }
  } else {
    size = s;
    data = (char*)malloc(size);
  }
}

void BitStream::isSpaceFor(size_t s) {
  if (s + c > size) {
    throw std::runtime_error("Out of space on BitStream");
  }
}

void BitStream::writeString(std::string s) {
  write<uint16_t>(s.size());
  for (int i = 0; i < s.size(); i++) write<char>(s[i]);
}

std::string BitStream::readString() {
  std::string s;
  uint16_t size = read<uint16_t>();
  s.resize(size);
  for (int i = 0; i < size; i++) s[i] = read<char>();
  return s;
}

ENetPacket* BitStream::createPacket(enet_uint32 flags) {
  return enet_packet_create(data, c, flags);
}
}  // namespace rdm::network
