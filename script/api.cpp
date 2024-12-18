#include "api.hpp"

#include <format>
#include <iomanip>
#include <sstream>

#include "script/my_basic.h"
namespace rdm::script {
static int _BasRealToStr(struct mb_interpreter_t* s, void** l) {
  float value;
  int precision = -1;
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_real(s, l, &value));
  if (mb_has_arg(s, l)) {
    mb_check(mb_pop_int(s, l, &precision));
  }
  mb_check(mb_attempt_close_bracket(s, l));

  std::string str;
  if (precision == -1) {
    str = std::format("{}", value);
  } else {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    str = ss.str();
  }
  mb_check(mb_push_string(s, l, mb_memdup(str.data(), str.size() + 1)));

  return MB_FUNC_OK;
}

void API::registerApi(struct mb_interpreter_t* s) {
  mb_begin_module(s, "XL");
  mb_register_func(s, "REALTOSTR", _BasRealToStr);
  mb_end_module(s);
}
}  // namespace rdm::script
