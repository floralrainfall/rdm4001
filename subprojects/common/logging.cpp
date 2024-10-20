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

  void Log::print(LogType t, const char* s, const std::source_location loc) {
  LogMessage m;
  m.t = t;
  m.message = std::string(s);
  m.loc = loc;
  singleton()->addLogMessage(m);
}

void Log::addLogMessage(LogMessage m) {
  if(m.t >= level) {
    char* clr = "";
    switch(m.t) {
    default:
    case LOG_EXTERNAL:
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
#ifndef NDEBUG
    ::printf("%s%s:%i: %s" COLOR_RESET "\n",clr,m.loc.file_name(),m.loc.line(),m.message.c_str());
#else
    ::printf("%s%s" COLOR_RESET "\n",clr,m.message.c_str());
#endif
  }

  log.push_front(m);
}
}
