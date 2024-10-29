#pragma once
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <typeinfo>
namespace rdm::network {
class BitStream {
  char* data;
  size_t size;
  size_t c;

  void makeSpaceFor(size_t s);

 public:
  BitStream();
  ~BitStream();

  BitStream(void* data, size_t size);
  BitStream(BitStream& stream);

  template <typename T>
  void write(T t) {
    makeSpaceFor(sizeof(T));
    memcpy(&data[c], &t, sizeof(T));
    c += sizeof(T);
    printf("%s\n", typeid(T).name());
  }

  template <typename T>
  T read() {
    T t;
    memcpy(&t, &data[c], sizeof(T));
    c += sizeof(T);
    printf("%s\n", typeid(T).name());
    return t;
  }
};
}  // namespace rdm::network
