#include "base_types.hpp"

namespace rdm::gfx {
  void BaseProgram::setParameter(std::string param, DataType type, Parameter parameter) {
    ParameterInfo pi;
    pi.type = type;
    pi.dirty = true;
    parameters[param] = std::pair<ParameterInfo, Parameter>(pi, parameter);
  }
}