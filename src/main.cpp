#include <coroutine>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include "uv.h"

template <typename T>
struct Task {

  struct promise_type {
    promise_type() {
    }

    ~promise_type() {
    }

    auto get_return_object() {

      return Task<T>(std::coroutine_handle<promise_type>::from_promise(*this)); // !! To communicate with the Task
    }

    auto initial_suspend() {
      return std::suspend_never{};
    }

    auto final_suspend() noexcept { // !! you forgot "noexcept"

      if (prev_) {
        prev_.resume();
      }

      return std::suspend_never{};
    }

    void unhandled_exception() {
      std::exit(1);
    }

    void return_value(T value) {
      value_ = value;
    }
    T value_;
    std::coroutine_handle<promise_type> prev_;
  };

  std::coroutine_handle<promise_type> waiting_; // !! To communicate with the promise_type

  Task(std::coroutine_handle<promise_type> waiting) : waiting_(waiting) { // !! save it
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }
  void await_suspend(std::coroutine_handle<promise_type> h) {

    waiting_.promise().prev_ = h;
  }

  T await_resume() noexcept {
    T value = std::move(waiting_.promise().value_);
    return value;
  }
};

uv_loop_t *loop;

struct TimerAwaitable {

  bool await_ready() {
    return false;
  }
  void await_suspend(std::coroutine_handle<> h) {
    // h.destroy();
    uv_timer_init(loop, &this->timer);
    uv_timer_start(&this->timer, (uv_timer_cb)&timer_cb1, 1000, 0);
    h_ = h;
  }

  void await_resume() {
  }

  static void timer_cb1(uv_timer_t *timer, int status) {
    printf("timer callback enter\n");
    TimerAwaitable *pTimerAwaitable = reinterpret_cast<TimerAwaitable *>(timer);

    pTimerAwaitable->h_.resume();

    delete pTimerAwaitable;

    printf("timer callback exit\n");
  }

  uv_timer_t timer;
  std::coroutine_handle<> h_;
};

Task<uint32_t> CDCStart() {

  printf("CDCStart enter\n");
  TimerAwaitable *pTimerAwaitable = new TimerAwaitable();

  co_await *pTimerAwaitable;

  printf("CDCStart return\n");
  co_return 5;
}

Task<uint32_t> wrapperStart() {
  printf("wrapperStart enter\n");
  Task task = CDCStart();
  uint32_t res = co_await task;

  printf("wrapperStart return %d\n", res);
  co_return res + 5;
}

int main() {

  loop = uv_default_loop();

  auto t = wrapperStart();

  uv_run(loop, UV_RUN_DEFAULT);

  return 0;
}