#include "script.hpp"

#include <stdarg.h>

#include "context.hpp"
#include "logging.hpp"
#include "my_basic.h"

namespace rdm::script {
static void _basError(struct mb_interpreter_t* s, mb_error_e e, const char* m,
                      const char* f, int p, unsigned short row,
                      unsigned short col, int abort_code) {
  LogType type = abort_code == MB_FUNC_WARNING ? LOG_WARN : LOG_ERROR;
  mb_unrefvar(s);
  mb_unrefvar(p);

  if (e == SE_NO_ERR) return;

  if (f) {
    if (e == SE_RN_REACHED_TO_WRONG_FUNCTION) {
      Log::printf(type,
                  "Ln %d, Col %d in Func: %s\n    Code %d, Abort Code %d\n    "
                  "Message: %s.",
                  row, col, f, e, abort_code, m);
    } else {
      Log::printf(type,
                  "Ln %d, Col %d in File: %s\n    Code %d, Abort Code %d\n    "
                  "Message: %s.",
                  row, col, f, e,
                  e == SE_EA_EXTENDED_ABORT ? abort_code - MB_EXTENDED_ABORT
                                            : abort_code,
                  m);
    }
  } else {
    Log::printf(
        type,
        "Ln %d, Col %d\n    Code %d, Abort Code %d\n    Message: "
        "%s.",
        row, col, e,
        e == SE_EA_EXTENDED_ABORT ? abort_code - MB_EXTENDED_ABORT : abort_code,
        m);
  }
}

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
    Log::printf(LOG_ERROR, "Encoding error.");
  } else if (result > (int)len) {
    len = result + 1;
    ptr = (char*)malloc(result + 1);
    result = vsnprintf(ptr, len, fmt, argptr);
  }
  va_end(argptr);
  if (result >= 0) {
    printf("%s\n", ptr);
  }
  if (ptr != buf) free(ptr);

  return result;
}

Script::Script(Context* context) {
  mb_open(&bas);
  context->setContext(this);
}

Script::~Script() {  // mb_close(&bas);
}

void Script::load(std::string s) {
  mb_load_string(bas, s.c_str(), true);
  mb_set_printer(bas, _basPrint);
  mb_set_error_handler(bas, _basError);
}

void Script::run() { mb_run(bas, true); }

void Script::initialize() { mb_init(); }

void Script::deinitialize() { mb_dispose(); }
}  // namespace rdm::script
