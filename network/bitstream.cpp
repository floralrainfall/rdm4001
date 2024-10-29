#include "bitstream.hpp"

#include <stdlib.h>
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
    if (s + c < size) {
      size *= 2;
      data = (char*)realloc(data, size);
    }
  } else {
    size = s;
    data = (char*)malloc(size);
  }
}
}  // namespace rdm::network
