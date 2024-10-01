#include "layout.hpp"

#include "logging.hpp"

namespace rdm::gfx::gui {
Layout::Layout() { positioning = Absolute; }

void Layout::fromJsonData(const json j) {
  name = j["Name"];
  json _elements = j["Elements"];
  if (j.contains("Positioning")) positioning = j["Positioning"];
  for (auto it = _elements.begin(); it != _elements.end(); ++it) {
    try {
      Element m = Element();
      if (positioning == Relative) m.maxScale = glm::vec2(1, 1);
      m.fromJsonData(*it);
      if (m.type == Element::Button) {
      }
      elements.push_back(m);
    } catch (std::exception& e) {
      Log::printf(LOG_ERROR, "Error creating element, what() = %s", e.what());
    }
  }
  this->j = j;

  update(glm::vec2(0));
}

void Layout::update(glm::vec2 framebufferSize) {
  if (j.contains("Grid")) {
    int numElements = elements.size();
    glm::vec2 gridSize;

    if (j.contains("GridSizeX")) {
      gridSize.x = j["GridSizeX"];
      gridSize.y = j["GridSizeY"];
      overrideSize = true;
    } else {
      gridSize = framebufferSize;
      overrideSize = false;
    }

    size = gridSize;
    glm::vec2 elemSize;
    if (j["Grid"] == "Vertical") {
      elemSize.x = gridSize.x;
      elemSize.y = gridSize.y / numElements;
    } else if (j["Grid"] == "Horizontal") {
      elemSize.x = gridSize.x / numElements;
      elemSize.y = gridSize.y;
    }

    bool negativeGrid = j.contains("GridGrow") && j["GridGrow"] == "Negative";
    glm::vec2 elemPos = glm::vec2(0);
    if (negativeGrid) {
      if (j["Grid"] == "Vertical") {
        elemPos.y += gridSize.y;
      } else if (j["Grid"] == "Horizontal") {
        elemPos.x += gridSize.x;
      }
    }
    for (int i = 0; i < numElements; i++) {
      Element& elem = elements[i];

      glm::vec2 scale, maxScale, minScale, offset;

      switch (positioning) {
        case Absolute:
          break;
        case Relative:
          scale = elem.origScale * size;
          maxScale = elem.origMaxScale * size;
          minScale = elem.origMinScale * size;
          offset = elem.origOffset * size;
          break;
      }

      if (!negativeGrid) {
        elem.offset = offset + elemPos;
      }

      glm::vec2 elemScale = glm::min(elemSize, maxScale);
      elemScale = glm::max(elemScale, minScale);
      elem.scale = elemScale;

      glm::vec2 growValue = elemScale;
      if (negativeGrid) {
        growValue = -growValue;
      }

      if (j["Grid"] == "Vertical") {
        elemPos.y += growValue.y;
      } else if (j["Grid"] == "Horizontal") {
        elemPos.x += growValue.x;
      } else if (j["Grid"] == "Rows") {
      }

      if (negativeGrid) {
        elem.offset = offset + elemPos;
      }
    }
  }
}
}  // namespace rdm::gfx::gui