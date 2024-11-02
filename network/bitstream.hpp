#pragma once
#include <enet/enet.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <typeinfo>
namespace rdm::network {
class BitStream {
  char* data;
  size_t size;
  size_t c;

  void isSpaceFor(size_t s);
  void makeSpaceFor(size_t s);

 public:
  BitStream();
  ~BitStream();

  void* getData() { return data; }
  size_t getSize() { return c; }

  BitStream(void* data, size_t size);
  BitStream(BitStream& stream);

  template <typename T>
  void write(T t) {
    makeSpaceFor(sizeof(T));
    memcpy(&data[c], &t, sizeof(T));
    c += sizeof(T);
  }

  template <typename T>
  T read() {
    isSpaceFor(sizeof(T));
    T t;
    memcpy(&t, &data[c], sizeof(T));
    c += sizeof(T);
    return t;
  }

  void writeString(std::string s);
  std::string readString();

  ENetPacket* createPacket(enet_uint32 flags);
};
}  // namespace rdm::network
