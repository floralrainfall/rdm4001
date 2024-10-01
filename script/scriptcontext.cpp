#include "scriptcontext.hpp"

#include "filesystem.hpp"
#include "logging.hpp"
namespace rdm::script {
ScriptContext::ScriptContext() {
  domain = mono_jit_init("ScriptContext");
  if (!domain) {
    Log::printf(LOG_ERROR, "mono_jit_init");
  }

  common::OptionalData od =
      common::FileSystem::singleton()->readFile("dat4/AssemblyMain.dll");
  if (!od) {
    Log::printf(LOG_FATAL, "AssemblyMain not found");
    return;
  }

  MonoImageOpenStatus status;
  MonoImage* image =
      mono_image_open_from_data((char*)od->data(), od->size(), false, &status);
  if (status != MONO_IMAGE_OK) {
    Log::printf(LOG_ERROR, "mono_image_open_from_data status = %i", status);
  }

  assembly = mono_assembly_load_from(image, "AssemblyMain.dll", &status);
  if (!assembly) {
    Log::printf(LOG_ERROR, "mono_assembly_load_from status = %i", status);
  }

  rdmLibrary = mono_class_from_name(image, "RDM", "Library");
  if (!rdmLibrary) {
    Log::printf(
        LOG_ERROR,
        "mono_class_from_name(image, \"RDM\", \"Library\") returned NULL");
  }
  MonoMethod* mainMethod =
      mono_class_get_method_from_name(rdmLibrary, "Main", 0);

  MonoObject* rv = mono_runtime_invoke(mainMethod, NULL, (void*[]){NULL}, NULL);
}

ScriptContext::~ScriptContext() {
  Log::printf(LOG_INFO, "cleaning up assembly");
  mono_jit_cleanup(domain);
}
}  // namespace rdm::script