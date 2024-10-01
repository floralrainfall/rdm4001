#pragma once
#include <glm/glm.hpp>
#include <string>

#include "element.hpp"
#include "gfx/base_types.hpp"
#include "json.hpp"
#include "signal.hpp"

using json = nlohmann::json;
namespace rdm::gfx::gui {
/**
 * @brief Layouts store Element's, and are required to have them be rendered.
 * They can optionally use a grid.
 *
 * JSON format:
 * - Name (required): The name of this Layout.
 * - Elements (required): A list of elements that will be added to the layout.
 * See Element
 * - Positioning: Corresponds to the enum. 0 is Positioning::Absolute, 1 is
 * Positioning::Relative
 * - Grid: Grid type, required for live resizing of elements. Can be "Vertical",
 * or "Horizontal".
 * - GridSizeX: Width of the grid.
 * - GridSizeY: Height of the grid.
 * - GridGrow: Setting it to Negative will cause the grid to grow in the
 * opposite direction.
 */
class Layout {
  std::vector<Element> elements;
  std::vector<Signal<>> buttonSignals;
  std::string name;

  json j;

 public:
  Layout();

  /**
   * @brief The positioning and scaling of a Grid.
   */
  enum Positioning {
    /**
     * @brief Coordinates are in range [0.0, 0.0] to [screen width, screen
     * height]
     *
     */
    Absolute,
    /**
     * @brief Coordinates are in range [0.0, 0.0] to [1.0, 1.0]
     *
     */
    Relative,
  };

  /**
   * @brief Signal, created by Element::Button elements.
   *
   * @param id The id of the signal. They are created in the order they are
   * added to the Layout
   * @return Signal<>& The returned signal.
   */
  Signal<>& getSignal(int id) { return buttonSignals[id]; }
  std::vector<Element>& getElements() { return elements; }
  void fromJsonData(const json data);
  std::string getName() { return name; };
  void update(glm::vec2 framebufferSize);
  glm::vec2 size;
  bool overrideSize;

  Positioning getPositioning() { return positioning; }

 private:
  Positioning positioning;
};
}  // namespace rdm::gfx::gui