#pragma once
#include <string>

#include "context.hpp"
#include "my_basic.h"
namespace rdm::script {
class Script {
  struct mb_interpreter_t* bas;

 public:
  Script(Context* context);
  ~Script();

  void run();

  void load(std::string s);

  struct mb_interpreter_t* getInterpreter() { return bas; }

  static void initialize();
  static void deinitialize();
};
};  // namespace rdm::script
