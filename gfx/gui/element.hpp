#pragma once
#include "json.hpp"
namespace rdm::gfx::gui {
class Element {
public:
  enum Type {
    Text,
    Panel,
    Image,
    Button
  };

  Element();
  void fromJsonData(const json data);
};
}