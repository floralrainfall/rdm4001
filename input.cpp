#include "input.hpp"

#include "SDL.h"
#include "SDL_keyboard.h"
#include "logging.hpp"
#include "settings.hpp"

#ifdef __linux
#include <signal.h>
#endif

namespace rdm {
#ifdef __linux
static void intHandler(int) {
  Log::printf(LOG_INFO, "Received SIGINT, sending Quit signal");

  InputObject quit;
  quit.type = InputObject::Quit;
  Input::singleton()->postEvent(quit);
}
#endif

Input::Input() {
  mouseLocked = false;
  mouseSensitivity = 2.0;
  mouseDelta = glm::vec3(0);
  mousePosition = glm::vec3(0);

  memset(keysDown, 0, sizeof(keysDown));
  memset(mouseButtonsDown, 0, sizeof(mouseButtonsDown));

#ifdef __linux
  signal(SIGINT, intHandler);
#endif
}

rdm::Input* _singleton = 0;
rdm::Input* Input::singleton() {
  if (!_singleton) {
    _singleton = new rdm::Input();
  }
  return _singleton;
}

void Input::startEditingText(bool clear) {
  if (clear) text.clear();
  SDL_StartTextInput();
}

void Input::stopEditingText() { SDL_StopTextInput(); }

std::string& Input::getEditedText() { return text; }

void Input::beginFrame() {
  std::scoped_lock lock(flushing);
  mouseDelta = glm::vec3(0.0);
}

void Input::postEvent(InputObject object) {
  std::scoped_lock lock(flushing);
  events.push_back(object);

  if (events.size() > 1000)
    Log::printf(LOG_WARN, "Too many events in event buffer (%i)",
                events.size());
}

void Input::flushEvents() {
  std::scoped_lock lock(flushing);
  while (events.size()) {
    InputObject event = events.front();
    bool keyPressed = false;
    switch (event.type) {
      case InputObject::Quit:
        quitSignal.fire(event);
        break;
      case InputObject::KeyPress:
        keyDownSignals[event.data.key.key].fire();
      case InputObject::KeyUp:
        keyPressed = event.type == InputObject::KeyPress;
        for (auto [name, _axis] : axis) {
          if (_axis.positive == event.data.key.key) {
            if (keyPressed) {
              if (_axis.value == 0.0) _axis.value = 1.0;
            } else {
              if (_axis.value == 1.0) _axis.value = 0.0;
            }
          } else if (_axis.negative == event.data.key.key) {
            if (keyPressed) {
              if (_axis.value == 0.0) _axis.value = -1.0;
            } else {
              if (_axis.value == -1.0) _axis.value = 0.0;
            }
          }
          axis[name] = _axis;
        }
        if (event.data.key.key < 255) {
          keysDown[event.data.key.key] = keyPressed;
        }
        break;
      case InputObject::MousePress:
        mousePressed.fire();
      case InputObject::MouseUp:
        keyPressed = event.type == InputObject::MousePress;
        mouseButtonsDown[event.data.mouse.button] = keyPressed;
        break;
      case InputObject::MouseMove:
        mousePosition.x = event.data.mouse.position[0];
        mousePosition.y = event.data.mouse.position[1];
        mouseDelta.x = ((float)event.data.mouse.delta[0]) / mouseSensitivity;
        mouseDelta.y = ((float)event.data.mouse.delta[1]) / mouseSensitivity;
        break;
      default:
        break;
    }

    onEvent.fire(event);
    events.pop_front();
  }
}

Input::Axis* Input::newAxis(std::string _axis, SDL_Keycode positive,
                            SDL_Keycode negative) {
  Axis x;
  x.value = 0.0;
  x.negative = negative;
  x.positive = positive;
  axis[_axis] = x;
  return &axis[_axis];
}

Input::Axis* Input::getAxis(std::string _axis) { return &axis[_axis]; }
}  // namespace rdm
