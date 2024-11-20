#pragma once
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>

#include "gfx/base_types.hpp"
#include "network/entity.hpp"
#include "network/network.hpp"
#include "signal.hpp"
namespace rt {
class America : public rdm::network::Entity {
  rdm::ClosureId closure;

 public:
  enum Location {
    SoCal,  // Start

    NorCal,  // West Route (thru Canada)
    Oregon,
    Washington,
    Vancouver,
    Alberta,
    Sasketchewan,
    Manitoba,
    Ontario,
    Ottawa,
    Montreal,
    NewBrunswick,

    Arizona,  // South Route (through the US South and the East Coast)
    NewMexico,
    Texas,
    Georgia,
    Virginia,
    NewYorkCity,
    Boston,

    OklahomaCity,  // Midwest Route
    Missouri,
    Iowa,
    Minnesota,
    Illinois,
    Michigan,

    NovaScotia,  // End
  };

  enum PathType { USA, Canada, Quebec, Midwest, Republican, Hot, USCIS };

  struct LocationInfo {
    std::string name;
    std::vector<std::pair<PathType, Location>> connectedLocations;
    glm::ivec2 mapPosition;

    // CLIENT only
    rdm::gfx::BaseTexture* logo;
  };

  std::map<Location, LocationInfo> locationInfo;

  void fillLocationInfo();

  America(rdm::network::NetworkManager* manager, rdm::network::EntityId id);
  virtual ~America();

  virtual void tick();

  virtual const char* getTypeName() { return "America"; }
};

}  // namespace rt
