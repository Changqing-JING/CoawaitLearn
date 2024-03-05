#include "TimerAwaitable.hpp"

namespace LibAwait {

void TimerAwaitable::timerCallback(uint64_t const ctx) noexcept {
  std::function<void(uint8_t)> *const pCallback = reinterpret_cast<std::function<void(uint8_t)> *>(ctx);
  (*pCallback)(0);
  delete pCallback;
}

uint64_t TimerAwaitable::setTimeoutWrapper(uint64_t const timeout, std::function<void(uint8_t)> callback) noexcept {
  std::function<void(uint8_t)> *const pCallback = new std::function<void(uint8_t)>(std::move(callback));
  return cdc::timer::setTimeout(timeout, timerCallback, reinterpret_cast<uintptr_t>(pCallback));
}

LibAwait::Awaitable<uint8_t> TimerAwaitable::waitForTimeout(uint32_t const timeout) noexcept {

  auto requestFunctionCreator = [timeout](std::function<void(uint8_t)> callback) noexcept -> std::function<void()> {
    return [timeout, callback = std::move(callback)]() noexcept {
      setTimeoutWrapper(timeout, callback);
    };
  };

  Awaitable<uint8_t> pAwaitable(std::move(requestFunctionCreator));
  return pAwaitable;
}

} // namespace LibAwait