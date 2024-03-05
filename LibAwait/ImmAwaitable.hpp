#ifndef IMM_AWAITABLE_HPP
#define IMM_AWAITABLE_HPP
#include "Awaitable.hpp"

namespace LibAwait {
template <typename T>
class ImmAwaitable {

public:
  explicit ImmAwaitable(T data) noexcept : data_(std::move(data)) {
  }

  inline bool await_ready() const noexcept {
    return true;
  }

  void await_suspend(std::coroutine_handle<> const h) const noexcept {
    static_cast<void>(h);
  }

  inline T await_resume() noexcept {
    return std::move(data_);
  }

private:
  T data_;
};
} // namespace LibAwait

#endif