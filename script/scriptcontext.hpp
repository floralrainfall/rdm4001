#pragma once
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
namespace rdm::script {
class ScriptContext {
  MonoDomain* domain;
  MonoAssembly* assembly;
  MonoClass* rdmLibrary;

 public:
  ScriptContext();
  ~ScriptContext();
};
}  // namespace rdm::script