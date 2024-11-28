#pragma once
#include <AL/al.h>
#include <AL/alc.h>

#include <optional>

#include "world.hpp"

namespace rdm {
class SoundManager;
struct Sound {
  enum LoadType {
    Buffer,
    Stream,
    Dynamic,
  };

  // loopingEnabled only useful on SoundStream
  virtual bool uploadData(ALuint buffer, bool loopingEnabled = false) = 0;
  virtual LoadType getLoadType() = 0;
};

class SoundEmitter {
  friend class SoundManager;

  SoundManager* parent;
  Sound* playingSound;

  bool looping;
  bool playing;
  float pitch;
  float gain;

  ALuint source;
  ALuint buffer;
  ALuint streamBuffer[10];

  SoundEmitter(SoundManager* manager);

 public:
  ~SoundEmitter();
  Sound* getCurrentSound();

  Graph::Node* node;

  void setLooping(bool looping);
  void setPitch(float pitch);
  void setGain(float gain);

  bool isPlaying() { return playing; }
  void play(Sound* sound);
  void stop();
  void service();
};

class SoundCache {
  friend class SoundManager;
  std::map<std::string, std::unique_ptr<Sound>> sounds;
  SoundManager* manager;

  SoundCache(SoundManager* manager);

 public:
  std::optional<Sound*> get(std::string name,
                            Sound::LoadType type = Sound::Buffer);
};

class SoundManager {
  World* world;
  ALCdevice* device;
  ALCcontext* context;
  ALCboolean contextMadeCurrent;
  std::unique_ptr<SoundCache> cache;

  std::vector<std::string> devices;
  std::vector<SoundEmitter*> emitters;

  void printError();

 public:
  SoundManager(World* world);
  ~SoundManager();

  Graph::Node* listenerNode;

  SoundEmitter* newEmitter();
  void delEmitter(SoundEmitter* emitter);

  SoundCache* getSoundCache() { return cache.get(); };

  void service();
};

}  // namespace rdm
