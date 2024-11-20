#pragma once
#include "pawn.hpp"

#include "gfx/engine.hpp"
#include "imgui.h"
#include "network/bitstream.hpp"
#include "roadtrip/america.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace rt {
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
            glm::vec3(point, 70.0) +
            glm::vec3(40 * sin(getGfxEngine()->getTime() * t),
                      40 * cos(getGfxEngine()->getTime() * t), 0.0));
        getGfxEngine()->getCamera().setUp(glm::vec3(0.0, 0.0, 1.0));

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

        ImGui::Begin("What to do today");
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
    desiredLocation = location;
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
      if (pathType == America::USCIS) {
        if (!inCanada) {
          caPointOfEntry = location;
          rdm::Log::printf(rdm::LOG_DEBUG, "POE = %i", caPointOfEntry);
        }
        rdm::Log::printf(rdm::LOG_DEBUG, "%s %s Canada",
                         displayName.get().c_str(),
                         inCanada ? "exited" : "entered");
        inCanada = !inCanada;
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
