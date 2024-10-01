#include "element.hpp"

namespace rdm::gfx::gui {
Element::Element() {
  dirty = true;
  maxScale = glm::vec2(INT_MAX, INT_MAX);
  minScale = glm::vec2(0, 0);
  offset = glm::vec2(0);
  uniqueTexture = nullptr;
}

void Element::fromJsonData(const json data) {
  dirty = true;
  type = (Type)data["Type"];
  if (data.contains("OffsetX")) offset.x = data["OffsetX"];
  if (data.contains("OffsetY")) offset.y = data["OffsetY"];
  origOffset = offset;
  switch (type) {
    case Text:
      if (data["Text"].is_string()) text = data["Text"];
      if (data.contains("Font"))
        font = data["Font"];
      else
        font = "dat3/default.ttf";
      if (data.contains("FontSize"))
        fontSize = data["FontSize"];
      else
        fontSize = 10;
      if (data.contains("Wrapped"))
        wrapped = data["Wrapped"];
      else
        wrapped = false;
      if (data.contains("MaxScaleX"))
        maxScale.x = data["MaxScaleX"];
      else
        maxScale.x = 100.0;
      origMaxScale = maxScale;
      override = true;
      break;
    case Image:
      if (data["Image"].is_string()) text = data["Image"];
    case Panel:
      if (data.contains("ScaleX")) scale.x = data["ScaleX"];
      if (data.contains("ScaleY")) scale.y = data["ScaleY"];
      origScale = scale;
      if (data.contains("MaxScaleX")) maxScale.x = data["MaxScaleX"];
      if (data.contains("MaxScaleY")) maxScale.y = data["MaxScaleY"];
      origMaxScale = maxScale;
      break;
    default:
      throw std::runtime_error("Bad type");
      break;
  }
}
}  // namespace rdm::gfx::gui