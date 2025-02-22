#pragma once
#include <cxxabi.h>

#include <functional>
#include <map>
#include <stdexcept>

#include "logging.hpp"

#ifndef DISABLE_EASY_PROFILER
#include <easy/profiler.h>
#endif

namespace rdm {
typedef size_t ClosureId;

ClosureId __newClosureId();

/**
 * @brief Signals. These are able to be fired, and will execute signal handlers
 * on the firing thread. These are not related to POSIX signals.
 *
 * @tparam Args The arguments of the signals. These will be required on
 * Signal::fire and can be retrieved on Signal::listen
 */
template <typename... Args>
class Signal {
 public:
  typedef std::function<void(Args...)> Function;

  /**
   * @brief Fires the signal.
   *
   * Will execute listeners on the firing thread, so don't rely on the fact that
   * you may add listeners from different threads.
   *
   * @param a The arguments to pass to signal listeners.
   */
  void fire(Args... a) {
#ifndef DISABLE_EASY_PROFILER
    EASY_FUNCTION();
#endif
    if (pendingClosures.size() != 0) {
#ifndef DISABLE_EASY_PROFILER
      EASY_BLOCK("Closures");
#endif
      for (auto closure : pendingClosures) {
        try {
          closure(a...);
        } catch (std::exception& e) {
          int status;
          Log::printf(LOG_ERROR, "Error calling closure for signal %s, '%s'",
                      abi::__cxa_demangle(typeid(this).name(), 0, 0, &status),
                      e.what());
        }
      }
      pendingClosures.clear();
    }

#ifndef DISABLE_EASY_PROFILER
    EASY_BLOCK("Listeners");
#endif
    for (auto listener : listeners) {
      try {
        listener.second(a...);
      } catch (std::exception& e) {
        int status;
        Log::printf(LOG_ERROR, "Error calling listener %i for signal %s, '%s'",
                    listener.first,
                    abi::__cxa_demangle(typeid(this).name(), 0, 0, &status),
                    e.what());
      }
    }
  };

  /**
   * @brief Adds a listener to the signal.
   *
   * Will be executed on the firing thread, not the one you added the listener
   * from.
   *
   * @param a The function to add.
   */
  ClosureId listen(Function a) {
    ClosureId id = __newClosureId();
    listeners[id] = a;
    return id;
  }

  void addClosure(Function a) { pendingClosures.push_back(a); }

  void removeListener(ClosureId id) {
    auto it = listeners.find(id);
    if (it != listeners.end())
      listeners.erase(it);
    else
      throw std::runtime_error("Removing invalid closure id");
  }

  size_t size() { return listeners.size(); }

 private:
  std::map<ClosureId, Function> listeners;
  std::vector<Function> pendingClosures;
};
}  // namespace rdm
