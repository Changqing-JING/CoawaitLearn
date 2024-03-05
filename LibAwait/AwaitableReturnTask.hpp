#ifndef LIB_AWAIT_TASK
#define LIB_AWAIT_TASK

#include <any>
#include <cassert>
#include <coroutine>

namespace LibAwait {

class AwaitableReturnTask {

public:
  class promise_type {
  public:
    inline AwaitableReturnTask get_return_object() noexcept {

      return AwaitableReturnTask(std::coroutine_handle<promise_type>::from_promise(*this)); // !! To communicate with the AwaitableReturnTask
    }

    inline auto initial_suspend() noexcept {
      return std::suspend_never{};
    }

    auto final_suspend() noexcept {
      if (prev_) {
        prev_.resume();
      }
      return std::suspend_never{};
    }

    // GCOVR_EXCL_START
    // WASM 1.0 doesn't use exception
    inline void unhandled_exception() noexcept {
    }
    // GCOVR_EXCL_STOP

    inline void return_value(std::any value) noexcept {
      assert(owner_ != nullptr); // NOLINT(clang-analyzer-core.UndefinedBinaryOperatorResult)
      owner_->value_ = std::move(value);
    }

    void setOwner(AwaitableReturnTask *owner) noexcept {
      owner_ = owner;
    }

  private:
    friend AwaitableReturnTask;
    std::coroutine_handle<promise_type> prev_;
    AwaitableReturnTask *owner_ = nullptr;
  };

  inline explicit AwaitableReturnTask(std::coroutine_handle<promise_type> handle) noexcept : handle_(handle) {
    handle_.promise().setOwner(this);
  }

  //  the three special methods that are called as part of a co_await
  inline constexpr bool await_ready() const noexcept {
    return true; // suspend_always
  }

  inline void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
    // suspend coro and store coroutine state
    handle_.promise().prev_ = h;
    // if return value is void or true, suspend sucessed. Controling right return to caller/resumer.
  }

  inline std::any await_resume() noexcept {
    return std::move(value_);
  }

private:
  std::coroutine_handle<promise_type> handle_; // !! To communicate with the promise_type
  std::any value_;
};
} // namespace LibAwait
#endif