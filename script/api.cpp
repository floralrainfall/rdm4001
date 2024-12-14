#include "api.hpp"

#include <format>

#include "script/my_basic.h"
namespace rdm::script {
static int _BasRealToStr(struct mb_interpreter_t* s, void** l) {
  float value;
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_real(s, l, &value));
  mb_check(mb_attempt_close_bracket(s, l));

  std::string str = std::format("{}", value);
  mb_check(mb_push_string(s, l, mb_memdup(str.data(), str.size() + 1)));

  return MB_FUNC_OK;
}

void API::registerApi(struct mb_interpreter_t* s) {
  mb_begin_module(s, "XL");
  mb_register_func(s, "REALTOSTR", _BasRealToStr);
  mb_end_module(s);
}
}  // namespace rdm::script
