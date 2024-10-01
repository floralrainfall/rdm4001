#pragma once
#include <enet/enet.h>

namespace rdm::network {
struct Player {
  enum Type {
    ConnectedPlayer,
    Machine,
    Unconnected,
    Undifferentiated,
  };

  ENetPeer* peer;
  Type type;
};
}