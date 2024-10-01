#include "logging.hpp"
#include <stdio.h>
#include <stdarg.h>

namespace rdm {
Log::Log() {
  level = LOG_DEBUG;
}

static Log* _singleton = 0;
Log* Log::singleton() {
  if(!_singleton)
    _singleton = new Log();
  return _singleton;
}

void Log::print(LogType t, const char* s) {
  LogMessage m;
  m.t = t;
  m.message = std::string(s);
  singleton()->addLogMessage(m);
}

void Log::printf(LogType t, const char* s, ...) {
  char buf[4096];
  va_list v;
  va_start(v, s);
  vsnprintf(buf, sizeof(buf), s, v);
  print(t, buf);
}

void Log::addLogMessage(LogMessage m) {
  if(m.t >= level) {
    char* clr = "";
    switch(m.t) {
    default:
    case LOG_DEBUG:
      clr = HBLK;
      break;
    case LOG_INFO:
      clr = WHT;
      break;
    case LOG_WARN:
      clr = YEL;
      break;
    case LOG_ERROR:
      clr = RED;
      break;
    case LOG_FATAL:
      clr = BRED;
      break;
    }
    ::printf("%s%s" COLOR_RESET "\n",clr,m.message.c_str());
  }

  log.push_front(m);
}
}