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

      return Task<T>(waiting_); // !! To communicate with the Task
    }

    auto initial_suspend() {
      return std::suspend_never{};
    }

    auto final_suspend() noexcept { // !! you forgot "noexcept"
      if (waiting_)
        waiting_.resume(); // !! resume anybody who is awaiting
      return std::suspend_never{};
    }

    void unhandled_exception() {
      std::exit(1);
    }

    void return_value(T value) {
      waiting_.promise().value_ = value;
    }
    std::coroutine_handle<promise_type> waiting_; // !! the awaiting coroutine
    T value_;
  };

  std::coroutine_handle<promise_type> &waiting_; // !! To communicate with the promise_type

  Task(std::coroutine_handle<promise_type> &waiting) : waiting_(waiting) { // !! save it
  }

  ~Task() {
  }

  constexpr bool await_ready() const noexcept {
    return false;
  }
  void await_suspend(std::coroutine_handle<promise_type> h) {

    waiting_ = h; // !! tell the promise_type who to resume when finished
  }

  void await_resume() const noexcept {
  }

  const T &get() const noexcept {
    return waiting_.promise().value_;
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

struct ST {
  ST() {
    printf("new ST\n");
  }

  ~ST() {
    printf("~ST\n");
  }
};

Task<uint32_t> CDCStart() {
  ST st;
  printf("CDCStart enter\n");
  TimerAwaitable *pTimerAwaitable = new TimerAwaitable();

  co_await *pTimerAwaitable;

  printf("CDCStart return\n");
  co_return 5;
}

Task<uint32_t> wrapperStart() {
  printf("wrapperStart enter\n");
  Task task = CDCStart();
  co_await task;
  uint32_t res = task.get();
  printf("wrapperStart return %d\n", res);
  // return;
}

int main() {

  loop = uv_default_loop();

  Task<uint32_t> t = wrapperStart();

  uv_run(loop, UV_RUN_DEFAULT);

  return 0;
}