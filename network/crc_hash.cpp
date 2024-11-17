#include "crc_hash.hpp"
namespace rdm::network {
Hash CRC32::hash(const void* buf, size_t size) {
  const uint8_t* p = (const uint8_t*)buf;
  Hash crc;

  crc = ~0U;
  while (size--) crc = CRC32Table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
  return crc ^ ~0U;
}
}  // namespace rdm::network
