#include "base_types.hpp"

#include <format>

#include "logging.hpp"

namespace rdm::gfx {
void BaseProgram::setParameter(std::string param, DataType type,
                               Parameter parameter) {
  ParameterInfo pi;
  pi.type = type;
  auto it = parameters.find(param);
  if (it != parameters.end()) {
    bool dirty = false;
    switch (type) {  // add comparisons here
    case DtSampler:
      dirty = true;  // TODO: add a dirty field to BaseTexture because
                     // destroyAndCreate will change the textures id
      break;
    case DtFloat:
      dirty = parameters[param].second.number != parameter.number;
      break;
    case DtInt:
      dirty = parameters[param].second.integer != parameter.integer;
      break;
    case DtVec3:
      dirty = parameters[param].second.vec3 != parameter.vec3;
      break;
    default:
      dirty = true;
      break;
    }
    pi.dirty = dirty;
    parameters[param] = std::pair<ParameterInfo, Parameter>(pi, parameter);
  } else {
    pi.dirty = true;
    parameters[param] = std::pair<ParameterInfo, Parameter>(pi, parameter);
  }
}

void BaseProgram::dbgPrintParameters() {
  int numDirty = 0;
  for(auto param : parameters) {
    if(!param.second.first.dirty)
      continue;
    std::string eql = "undefined";
    switch(param.second.first.type) {
    case DtFloat:
      eql = std::to_string(param.second.second.number);
      break;
    case DtInt:
      eql = std::to_string(param.second.second.integer);
      break;
    case DtVec3:
      eql = std::format("({}, {}, {})",
			param.second.second.vec3.x,
			param.second.second.vec3.y,
			param.second.second.vec3.z);
    default:
      break;
    }
    numDirty++;
    Log::printf(LOG_DEBUG, "%s (%i) = %s", param.first.c_str(), param.second.first.type, eql.c_str());
  }
  Log::printf(LOG_DEBUG, "%i", numDirty);
}
}  // namespace rdm::gfx
