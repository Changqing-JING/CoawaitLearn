#ifndef LIB_AWAIT_AWAITABLE_HPP
#define LIB_AWAIT_AWAITABLE_HPP

#include <coroutine>
#include <functional>
#include "AwaitableReturnTask.hpp"
#include "types/env.hpp"

namespace LibAwait {

template <typename T>
class Awaitable {

public:
  using RequestCallbackFunction = std::function<void(T)>;

  using RequestGenerateFunction = std::function<std::function<void()>(RequestCallbackFunction)>;

  explicit Awaitable(RequestGenerateFunction requestGenerateFunction) noexcept : requestGenerateFunction_(std::move(requestGenerateFunction)) {
  }

  inline bool await_ready() const noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<> const h) noexcept {
    h_ = h;
    std::function<void()> const requestFunction = requestGenerateFunction_([this](T data) noexcept {
      asyncResume(std::move(data));
    });
    requestFunction();
  }

  inline T await_resume() noexcept {
    return std::move(data_);
  }

protected:
  void asyncResume(T data) noexcept {
    data_ = std::move(data);
    h_.resume();
  }

private:
  std::coroutine_handle<> h_;
  RequestGenerateFunction requestGenerateFunction_;
  T data_;
};

} // namespace LibAwait

#endif