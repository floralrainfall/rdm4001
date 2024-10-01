#pragma once
#include <glm/glm.hpp>

#include "gfx/base_types.hpp"
#include "json.hpp"
using json = nlohmann::json;
namespace rdm::gfx::gui {
/**
 * @brief Elements are the base unit of any GUI display.
 * Offset and Scale will not be useful on Grid layouts, because the layout will
 * override those variables.
 *
 * JSON format:
 * - Type (required): The type of the element. This is a number that corresponds
 * to an enum, see Element::Type
 * - OffsetX: The X offset of the element. Uses parent layout's Positioning.
 * - OffsetY: The Y offset of the element. Uses parent layout's Positioning.
 * - ScaleX: The X scale of the element.
 * - ScaleY: The Y scale of the element.
 * - MinScaleX: The minimum scale of the element.
 * - MinScaleY: The minimum scale of the element.
 * - MaxScaleX: The maximum scale of the element.
 * - MaxScaleY: The maximum scale of the element.
 *
 * For Element::Text:
 * - Text (required): The text to be rendered.
 *
 * For Element::Image:
 * - Image (required): The path to the image, from the root of any archives.
 */
class Element {
  bool dirty;
  std::string text;

 public:
  enum Type {
    /**
     * @brief Text. This will use SDL_ttf
     *
     */
    Text,
    /**
     * @brief Panel.
     *
     */
    Panel,
    /**
     * @brief Image. Picture
     *
     */
    Image,
    /**
     * @brief Button. Will add a signal to the parent Layout, see
     * Layout::getSignal
     *
     */
    Button
  };

  Element();
  void fromJsonData(const json data);

  void setText(std::string text) {
    this->text = text;
    dirty = true;
  }
  std::string getText() { return this->text; }

  bool isDirty() { return dirty; }
  void clearDirty() { dirty = false; }

  std::string font;
  int fontSize;
  BaseTexture* uniqueTexture;
  bool wrapped;

  glm::vec2 offset;
  glm::vec2 origOffset;
  glm::vec2 scale;
  glm::vec2 origScale;  // do not modify
  glm::vec2 maxScale;
  glm::vec2 origMaxScale;  // do not modify
  glm::vec2 minScale;
  glm::vec2 origMinScale;  // do not modify
  Type type;
  bool override;
};
}  // namespace rdm::gfx::gui