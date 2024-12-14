#pragma once
#include "script/my_basic.h"

namespace rdm::gfx::gui {
class API {
 public:
  static void registerApi(struct mb_interpreter_t* s);
};
};  // namespace rdm::gfx::gui
