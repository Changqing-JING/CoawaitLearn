#ifndef TIMER_AWAITABLE_HPP
#define TIMER_AWAITABLE_HPP

#include <coroutine>
#include "Awaitable.hpp"
#include "AwaitableReturnTask.hpp"
#include "types/env.hpp"

namespace LibAwait {

class TimerAwaitable final {

public:
  static constexpr uint8_t waitForTimeoutRet = 0;
  static Awaitable<uint8_t> waitForTimeout(uint32_t const timeout) noexcept;

private:
  static uint64_t setTimeoutWrapper(uint64_t const timeout, std::function<void(uint8_t)> callback) noexcept;

  static void timerCallback(uint64_t ctx) noexcept;
};

} // namespace LibAwait

#endif