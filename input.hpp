#pragma once
#include <deque>
#include <mutex>
#include <string>
#include <map>
#include "signal.hpp"
#include <SDL2/SDL_keycode.h>

namespace rdm {
  struct InputObject {
    enum Type {
      KeyPress,
      KeyUp,
      MouseMove,
      MousePress,
      MouseUp,
      MouseScroll,
      Quit,
      TextEdit,
    };

    Type type;
    union {
      struct {
        SDL_Keycode key;
        char sym;
        SDL_Keymod mod;
      } key;
      struct {
        int delta[2];
        int position[2];
        int global_position[2];
        bool pressed[3]; // mb 1 = 0
        int wheel;
      } mouse;
    } data;
  };


  class Input {
    Input();

    std::mutex flushing;
    std::deque<InputObject> events;
  public:
    static Input* singleton();

    void postEvent(InputObject object);
    void flushEvents();

    struct Axis {
      float value;
      SDL_Keycode positive;
      SDL_Keycode negative;
    };

    Axis* newAxis(std::string axis, SDL_Keycode positive, SDL_Keycode negative);
    Axis* getAxis(std::string axis);

    Signal<InputObject> quitSignal;
    Signal<InputObject> onEvent;
  private:
    std::map<std::string, Axis> axis;
  };
}