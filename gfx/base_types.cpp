#include "base_types.hpp"

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
      default:
        dirty = true;
        break;
    }
    parameters[param] = std::pair<ParameterInfo, Parameter>(pi, parameter);
  } else {
    pi.dirty = true;
    parameters[param] = std::pair<ParameterInfo, Parameter>(pi, parameter);
  }
}
}  // namespace rdm::gfx