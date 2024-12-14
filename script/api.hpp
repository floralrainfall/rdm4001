#pragma once
#include "script/my_basic.h"
namespace rdm::script {
class API {
 public:
  static void registerApi(struct mb_interpreter_t* s);
};
};  // namespace rdm::script
