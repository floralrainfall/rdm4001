#pragma once
#include <functional>

namespace rdm {
/**
 * @brief Signals. These are able to be fired, and will execute signal handlers
 * on the firing thread. These are not related to POSIX signals.
 *
 * @tparam Args The arguments of the signals. These will be required on
 * Signal::fire and can be retrieved on Signal::listen
 */
template <typename... Args>
class Signal {
  std::vector<std::function<void(Args...)>> listeners;

 public:
  /**
   * @brief Fires the signal.
   *
   * Will execute listeners on the firing thread, so don't rely on the fact that
   * you may add listeners from different threads.
   *
   * @param a The arguments to pass to signal listeners.
   */
  void fire(Args... a) {
    for (auto listener : listeners) {
      listener(a...);
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
  void listen(std::function<void(Args...)> a) { listeners.push_back(a); }
};
}  // namespace rdm