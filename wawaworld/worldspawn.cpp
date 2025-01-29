#pragma once
#include "worldspawn.hpp"

#include <cstdio>
#include <format>

#include "console.hpp"
#include "fun.hpp"
#include "game.hpp"
#include "map.hpp"
#include "network/bitstream.hpp"
#include "physics.hpp"
#include "putil/fpscontroller.hpp"
#include "settings.hpp"
#include "world.hpp"
#include "wplayer.hpp"
namespace ww {
static rdm::CVar sv_nextmap("sv_nextmap", "ffa_naamda",
                            CVARF_NOTIFY | CVARF_SAVE | CVARF_REPLICATE);

static rdm::ConsoleCommand changelevel(
    "changelevel", "changelevel [map]", "changes level to map",
    [](Game* game, ConsoleArgReader r) {
      Worldspawn* worldspawn = dynamic_cast<Worldspawn*>(
          game->getServerWorld()->getNetworkManager()->findEntityByType(
              "Worldspawn"));
      if (worldspawn) {
        worldspawn->setNextMap(r.next());
        worldspawn->endRound();
      }
    });

Worldspawn::Worldspawn(net::NetworkManager* manager, net::EntityId id)
    : net::Entity(manager, id) {
  file = NULL;
  entity = NULL;
  roundStartTime = 0.f;
  mapName = "";
  nextMapName = sv_nextmap.getValue();
  currentStatus = Unknown;
  nextSpawnLocation = 0;
  if (!getManager()->isBackend()) {
    worldJob = getWorld()->stepped.listen([this] {
      std::scoped_lock lock(mutex);
      if (file) file->updatePosition(getGfxEngine()->getCamera().getPosition());
    });
    gfxJob = getGfxEngine()->renderStepped.listen([this] {
      if (currentStatus == RoundBeginning || currentStatus == RoundEnding) {
        ImGui::Begin("Round");
        ImGui::Text("Time until round %s: %0.1f",
                    currentStatus == RoundEnding ? "ending" : "starting",
                    roundStartTime - getManager()->getDistributedTime());
        for (auto& peer : getManager()->getPeers()) {
          if (!peer.second.playerEntity) continue;
          ImGui::Text("%s (%ims, %i loss)",
                      peer.second.playerEntity->displayName.get().c_str(),
                      peer.second.roundTripTime, peer.second.packetLoss);
        }
        ImGui::End();
      }
    });
    emitter.reset(getGame()->getSoundManager()->newEmitter());
  } else {
    setStatus(WaitingForPlayer);
  }
}

Worldspawn::~Worldspawn() {
  if (!getManager()->isBackend()) {
    if (entity) getGfxEngine()->deleteEntity(entity);
    getWorld()->stepped.removeListener(worldJob);
    getGfxEngine()->renderStepped.removeListener(gfxJob);
  }

  if (file) destroyFile();
}

void Worldspawn::loadFile(const char* _file) {
  std::scoped_lock lock(mutex);
  file = new BSPFile(mapPath(_file).c_str());
  mapName = _file;

  getWorld()->getPhysicsWorld()->physicsStepping.addClosure([this] {
    std::scoped_lock lock(mutex);
    file->addToPhysicsWorld(getWorld()->getPhysicsWorld());
  });

  if (getManager()->isBackend()) {
    std::vector<BSPEntity> entities = file->getEntities();
    for (auto entity : entities) {
      if (entity.properties["classname"] == "info_player_deathmatch") {
        glm::vec3 p = Math::stringToVec4(entity.properties["origin"]);
        mapSpawnLocations.push_back(p);
        Log::printf(LOG_DEBUG, "new spawn location = %f, %f, %f", p.x, p.y,
                    p.z);
      }
    }
  }
}

void Worldspawn::destroyFile() {
  if (!getManager()->isBackend()) {
    getGfxEngine()->deleteEntity(entity);
  }

  BSPFile* file = this->file;
  this->file = NULL;
  if (getWorld()->getRunning()) {
    PhysicsWorld* world =
        getWorld()->getPhysicsWorld();  // by the time physicsStepping or
                                        // renderStepped is called getManager()
                                        // is unusable
    gfx::Engine* gfxEngine = getGfxEngine();
    bool backend = getManager()->isBackend();
    world->physicsStepping.addClosure([this, file, world, backend, gfxEngine] {
      file->removeFromPhysicsWorld(world);
      if (!backend) {
        gfxEngine->renderStepped.addClosure([this, file] { delete file; });
      }
    });
  } else {
    delete file;
  }
}

static CVar sv_roundstarttime("sv_roundstarttime", "30.0", CVARF_SAVE);

void Worldspawn::setStatus(Status s) {
  switch (s) {
    case RoundBeginning:
      roundStartTime =
          getManager()->getDistributedTime() + sv_roundstarttime.getFloat();
      break;
    case RoundEnding:
      roundStartTime = getManager()->getDistributedTime() + 1.0;
      break;
    case InGame:
      loadFile(mapName.c_str());
      break;
    case RoundEnded:
      destroyFile();
      break;
    default:
      break;
  }
  currentStatus = s;
  getManager()->addPendingUpdate(getEntityId());
}

void Worldspawn::endRound() { setStatus(RoundEnding); }

void Worldspawn::tick() {
  if (getManager()->isBackend())
    if (nextMapName.empty()) nextMapName = sv_nextmap.getValue();
  switch (currentStatus) {
    default:
      break;
    case RoundBeginning:
      if (!getManager()->isBackend()) {
        if (!emitter->isPlaying())
          emitter->play(getGame()
                            ->getSoundManager()
                            ->getSoundCache()
                            ->get("dat5/mus/round_lobby.mp3", Sound::Stream)
                            .value());
      } else {
        getWorld()->setTitle("RDM: Lobby");

        auto ents = getManager()->findEntitiesByType("WPlayer");
        if (ents.size() == 0) setStatus(WaitingForPlayer);

        if (roundStartTime < getManager()->getDistributedTime()) {
          // start game
          mapName = nextMapName;
          nextMapName = "";
          setStatus(InGame);

          for (auto ent : ents) {
            WPlayer* player = dynamic_cast<WPlayer*>(ent);
            rdm::putil::FpsController* controller = player->getController();
            controller->teleport(spawnLocation());
            getManager()->addPendingUpdateUnreliable(player->getEntityId());
          }
        }
      }
      break;
    case WaitingForPlayer:
      if (!getManager()->isBackend()) {
        throw std::runtime_error(
            "WaitingForPlayer on client worldspawn??? IMPOSSIBLE");
      } else {
        getWorld()->setTitle("RDM: Waiting for players");
        auto players = getManager()->findEntitiesByType("WPlayer");
        if (players.size() != 0) setStatus(RoundBeginning);
      }
      break;
    case InGame:
      if (!getManager()->isBackend())
        emitter->stop();
      else {
        getWorld()->setTitle("RDM: " + mapName);
      }
      break;
    case RoundEnding:
      if (getManager()->isBackend()) {
        getWorld()->setTitle("RDM: Finishing up...");

        if (roundStartTime < getManager()->getDistributedTime()) {
          setStatus(RoundEnded);
        }
      }
      break;
    case RoundEnded:
      if (getManager()->isBackend()) {
        setStatus(RoundBeginning);
      }
      break;
  }
}

void Worldspawn::serialize(net::BitStream& stream) {
  std::scoped_lock lock(mutex);

  stream.write<Status>(currentStatus);
  switch (currentStatus) {
    case InGame:
      stream.writeString(mapName);
      break;
    case RoundEnding:
    case RoundBeginning:
      stream.write<float>(roundStartTime);
      break;
    default:
      break;
  }
}

glm::vec3 Worldspawn::spawnLocation() {
  if (!mapSpawnLocations.size()) {
    return glm::vec3(0, 0, 0);
  }
  int id = nextSpawnLocation;
  nextSpawnLocation = rand() % mapSpawnLocations.size();
  glm::vec3 location = mapSpawnLocations[id];
  Log::printf(LOG_DEBUG, "spawn location %i = %f, %f, %f", id, location.x,
              location.y, location.z);
  return location;
}

std::string Worldspawn::mapPath(std::string name) {
  return std::format("dat5/baseq3/maps/{}.bsp", name);
}

void Worldspawn::deserialize(net::BitStream& stream) {
  std::scoped_lock lock(mutex);

  currentStatus = stream.read<Status>();
  switch (currentStatus) {
    case InGame: {
      std::string newMap = stream.readString();
      if (!newMap.empty() && mapName != newMap) {
        mapName = newMap;
        file = new BSPFile(mapPath(mapName).c_str());
        getWorld()->getPhysicsWorld()->physicsStepping.addClosure([this] {
          std::scoped_lock lock(mutex);
          file->addToPhysicsWorld(getWorld()->getPhysicsWorld());
        });

        getGfxEngine()->renderStepped.addClosure([this] {
          std::scoped_lock lock(mutex);
          file->initGfx(getGfxEngine());
          entity = getGfxEngine()->addEntity<MapEntity>(file);
        });

        pendingAddToGfx = true;
      }
    } break;
    case RoundEnding:
    case RoundBeginning:
      roundStartTime = stream.read<float>();
      break;
    case RoundEnded:
      destroyFile();
    default:
      break;
  }
}
}  // namespace ww
