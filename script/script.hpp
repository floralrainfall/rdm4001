#pragma once
#include <string>

#include "my_basic.h"
namespace rdm::script {
class Script {
  struct mb_interpreter_t* bas;

 public:
  Script();
  ~Script();

  void run();

  void load(std::string s);

  struct mb_interpreter_t* getInterpreter() { return bas; }

  static void initialize();
  static void deinitialize();
};
};  // namespace rdm::script
