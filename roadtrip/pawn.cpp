#pragma once
#include "pawn.hpp"

#include "gfx/engine.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "network/bitstream.hpp"
#include "roadtrip/america.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace rt {
const char* PawnItem::name(Type t) {
  const char* names[] = {
      "Water Bottle",
      "Food (K-Ration)",
      "Dog (Golden Retriever)",
      "Knife",
      "Gun",
      "US Passport",
      "Canadian Passport",
      "Forged Canadian Passport",
      "Temporary Worker",
  };
  return names[t];
}

int PawnItem::cost(Type t) {
  int costs[] = {
      5, 6, 12, 5, 17, 2, 10, 30, 20,
  };
  return costs[t];
}

Pawn::Pawn(rdm::network::NetworkManager* manager, rdm::network::EntityId id)
    : rdm::network::Player(manager, id) {
  location = America::SoCal;
  desiredLocation = America::SoCal;
  turnEnded = false;
  vacationed = false;
  inCanada = false;
  cash = 37;

  if (!manager->isBackend()) {
    gfxTick = manager->getGfxEngine()->renderStepped.listen([this] {
      America* america =
          dynamic_cast<America*>(getManager()->findEntityByType("America"));
      if (america) {
        glm::ivec2 point = america->locationInfo[location].mapPosition;
        point.x = 963 - point.x;

        std::shared_ptr<rdm::gfx::Material> material =
            getGfxEngine()->getMaterialCache()->getOrLoad("Mesh").value();
        rdm::gfx::BaseProgram* program =
            material->prepareDevice(getGfxEngine()->getDevice(), 0);
        program->setParameter(
            "model", rdm::gfx::DtMat4,
            rdm::gfx::BaseProgram::Parameter{
                .matrix4x4 = glm::translate(glm::vec3(point, 0.0))});
        rdm::gfx::Model* model =
            getGfxEngine()->getMeshCache()->get("dat6/pawn.obj").value();
        model->render(getGfxEngine()->getDevice());

        if (!isLocalPlayer()) return;

        getGfxEngine()->getCamera().setPosition(glm::vec3(point, 0.0));
        float t = 0.1;
        getGfxEngine()->getCamera().setTarget(
            glm::vec3(point, 500.0) +
            glm::vec3(80 /*40 * sin(getGfxEngine()->getTime() * t)*/,
                      80 /*40 * cos(getGfxEngine()->getTime() * t)*/, 0.0));
        getGfxEngine()->getCamera().setUp(glm::vec3(0.0, 0.0, 1.0));
        getGfxEngine()->getCamera().setFOV(35.f);

        if (vacationed) {
          ImGui::Begin("You Win!");
          ImGui::Text("You have reached the vacation in Nova Scotia.");
          ImGui::End();
          return;
        }

        ImGui::Begin("The Sights");
        if (america->locationInfo[location].logo) {
          ImGui::Image(america->locationInfo[location].logo->getImTextureId(),
                       ImVec2(453, 339), {0, 1}, {1, 0});
        } else {
          ImGui::Text("Sorry nothing");
        }
        ImGui::End();

        ImGui::Begin("Stats");
        ImGui::Text("You have %i kroner", cash);
        if (inCanada) {
          ImGui::Image(getGfxEngine()
                           ->getTextureCache()
                           ->getOrLoad2d("dat6/canada.png")
                           .value()
                           .second->getImTextureId(),
                       ImVec2(106, 53), {0, 1}, {1, 0});
          ImGui::Text("You are in Canada");
        } else {
          ImGui::Image(getGfxEngine()
                           ->getTextureCache()
                           ->getOrLoad2d("dat6/usa.png")
                           .value()
                           .second->getImTextureId(),
                       ImVec2(100, 53), {0, 1}, {1, 0});
          ImGui::Text("You are in the United States of America");
        }
        ImGui::End();

        if (turnEnded) {
          ImGui::Begin("Please Wait");
          ImGui::Text("Turn ended. Waiting for other players...");
          ImGui::Separator();
          auto players = getManager()->findEntitiesByType(getTypeName());
          for (auto player : players) {
            Pawn* pawn = dynamic_cast<Pawn*>(player);
            ImGui::Text("%s... %s", pawn->displayName.get().c_str(),
                        pawn->turnEnded ? "ready" : "thinking");
          }

          ImGui::End();
          return;
        }

        static enum { Main, Shop } menuMode;

        ImGui::Begin("What to do today");
        switch (menuMode) {
          case Shop:
            static std::map<PawnItem::Type, int> cart;

            for (int i = 0; i < PawnItem::Max; i++) {
              if (cart[(PawnItem::Type)i]) continue;
              if (inCanada && i == PawnItem::Gun) {
                ImGui::Text("Guns are illegal in Canada");
                ImGui::Separator();
                continue;
              }
              ImGui::PushID(i);
              ImGui::Text("%s, %ikr", PawnItem::name((PawnItem::Type)i),
                          PawnItem::cost((PawnItem::Type)i));
              if (ImGui::Button("Add to Cart")) {
                cart[(PawnItem::Type)i] = true;
              }
              ImGui::Separator();
              ImGui::PopID();
            }

            ImGui::Begin("Cart");
            {
              int cost = 0;
              for (auto t : cart) {
                if (t.second) {
                  ImGui::PushID(t.first);
                  ImGui::Text("%s", PawnItem::name(t.first));
                  cost += PawnItem::cost(t.first);
                  if (ImGui::Button("Remove")) {
                    cart[t.first] = false;
                  }
                  ImGui::Separator();
                  ImGui::PopID();
                }
              }
              if (cost == 0) {
                ImGui::Text("Please select some items friend");
              } else {
                ImGui::Text("Buying will end your turn");
                if (cost < cash)
                  if (ImGui::Button("Buy")) {
                    menuMode = Main;
                    turnEnded = true;
                    for (auto t : cart) {
                      wantedItems.push_back(t.first);
                    }
                    cart.clear();
                    getManager()->addPendingUpdate(getEntityId());
                  }
                ImGui::Text("Total cost: %ikr", cost);
                if (cost > cash) {
                  ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0),
                                     "You do not have enough kroner");
                }
              }
            }
            ImGui::End();

            if (ImGui::Button("Back")) {
              cart.clear();
              menuMode = Main;
            }
            break;
          case Main:
            ImGui::Text("Where I am: %s",
                        america->locationInfo[location].name.c_str());
            ImGui::Separator();
            for (int i = 0;
                 i < america->locationInfo[location].connectedLocations.size();
                 i++) {
              ImGui::PushID(i);
              auto it = america->locationInfo[location].connectedLocations[i];
              America::LocationInfo info = america->locationInfo[it.second];
              ImGui::Text("%s", info.name.c_str());
              if (it.second == desiredLocation) {
                ImGui::Text("You are heading here next turn");
              } else {
                if (ImGui::Button("Go")) {
                  desiredLocation = it.second;
                }
              }
              ImGui::PopID();
            }
            ImGui::Separator();
            if (ImGui::Button("End Turn")) {
              turnEnded = true;
              getManager()->addPendingUpdate(getEntityId());  // sync changes
            }
            if (ImGui::Button("Shop")) {
              menuMode = Shop;
            }
        }
        ImGui::End();
      }
    });
  }
}

Pawn::~Pawn() {
  if (!getManager()->isBackend())
    getGfxEngine()->renderStepped.removeListener(gfxTick);
}

void Pawn::tick() {
  America* america =
      dynamic_cast<America*>(getManager()->findEntityByType("America"));
  if (america) {
  }
}

void Pawn::serialize(rdm::network::BitStream& stream) {
  Player::serialize(stream);

  if (getManager()->isBackend()) {
    stream.write<int>(cash);
    stream.write<bool>(turnEnded);
    stream.write<America::Location>(location);
    stream.write<bool>(vacationed);
    stream.write<bool>(inCanada);

    stream.write<int>(events.size());  // write events
    for (auto& event : events) {
      stream.write<PawnEvent::Type>(event.type);
      stream.writeString(event.eventName);
      stream.writeString(event.eventDesc);
    }
    events.clear();
  } else {
    stream.write<bool>(turnEnded);
    stream.write<America::Location>(desiredLocation);
  }
}

void Pawn::deserialize(rdm::network::BitStream& stream) {
  Player::deserialize(stream);

  if (getManager()->isBackend()) {
    turnEnded = stream.read<bool>();
    desiredLocation = stream.read<America::Location>();
  } else {
    cash = stream.read<int>();
    turnEnded = stream.read<bool>();
    location = stream.read<America::Location>();
    vacationed = stream.read<bool>();
    inCanada = stream.read<bool>();
    desiredLocation = location;

    int eventCount = stream.read<int>();
    events.clear();
    for (int i = 0; i < eventCount; i++) {
      PawnEvent event;
      event.type = stream.read<PawnEvent::Type>();
      event.eventName = stream.readString();
      event.eventDesc = stream.readString();
      events.push_back(event);
    }
  }
}

void Pawn::endTurn() {
  if (!getManager()->isBackend()) throw std::runtime_error("");
  America* america =
      dynamic_cast<America*>(getManager()->findEntityByType("America"));

  if (desiredLocation != location) {
    America::LocationInfo info = america->locationInfo[location];
    bool pathAllowed = false;
    America::PathType pathType;
    for (auto path : info.connectedLocations) {
      if (path.second == desiredLocation) {
        pathAllowed = true;
        pathType = path.first;
        break;
      }
    }

    if (pathAllowed) {
      switch (pathType) {
        case America::USCIS:  // BORDER patrol
          if (!inCanada) {
            caPointOfEntry = location;
            rdm::Log::printf(rdm::LOG_DEBUG, "POE = %i", caPointOfEntry);
          } else {
          }

          rdm::Log::printf(rdm::LOG_DEBUG, "%s %s Canada",
                           displayName.get().c_str(),
                           inCanada ? "exited" : "entered");
          inCanada = !inCanada;
          break;
        default:
          break;
      }

      location = desiredLocation;
    } else {
      rdm::Log::printf(rdm::LOG_DEBUG, "%s tried to move to an invalid place",
                       displayName.get().c_str());
    }

    if (location == America::NovaScotia) {  // won game
      vacationed = true;
      rdm::Log::printf(rdm::LOG_INFO, "%s reached the vacation",
                       displayName.get().c_str());
    }
  }

  turnEnded = false;

  getManager()->addPendingUpdate(getEntityId());
}
}  // namespace rt
