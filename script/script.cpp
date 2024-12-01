#include "script.hpp"

#include <stdarg.h>

#include "logging.hpp"
#include "my_basic.h"

namespace rdm::script {

static int _basPrint(struct mb_interpreter_t* s, const char* fmt, ...) {
  char buf[64];
  char* ptr = buf;
  size_t len = sizeof(buf);
  int result = 0;
  va_list argptr;
  mb_unrefvar(s);

  va_start(argptr, fmt);
  result = vsnprintf(ptr, len, fmt, argptr);
  if (result < 0) {
    fprintf(stderr, "Encoding error.\n");
  } else if (result > (int)len) {
    len = result + 1;
    ptr = (char*)malloc(result + 1);
    result = vsnprintf(ptr, len, fmt, argptr);
  }
  va_end(argptr);
  if (result >= 0) {
    Log::printf(LOG_DEBUG, "%s", ptr);
  }
  if (ptr != buf) free(ptr);

  return result;
}

Script::Script() {
  mb_open(&bas);
  mb_set_printer(bas, _basPrint);
}

Script::~Script() {  // mb_close(&bas);
}

void Script::load(std::string s) { mb_load_string(bas, s.c_str(), true); }

void Script::run() { mb_run(bas, true); }

void Script::initialize() { mb_init(); }

void Script::deinitialize() { mb_dispose(); }
}  // namespace rdm::script
