#include <any>
#include <cassert>
#include <coroutine>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>

bool ready = true;

class AwaitableReturnTask {

public:
  class promise_type {
  public:
    promise_type() {
      std::cout << "new promise_type " << this << std::endl;
    }
    inline AwaitableReturnTask get_return_object() noexcept {
      std::cout << "promise_type::get_return_object " << this << std::endl;
      return AwaitableReturnTask(std::coroutine_handle<promise_type>::from_promise(*this)); // !! To communicate with the AwaitableReturnTask
    }

    inline auto initial_suspend() noexcept {
      std::cout << "promise_type::initial_suspend " << this << std::endl;
      return std::suspend_never{};
    }

    auto final_suspend() noexcept {
      std::cout << "promise_type::final_suspend " << this << std::endl;
      if (prev_) {
        std::cout << "promise_type::final_suspend prev_.resume" << std::endl;
        prev_.resume();
      } else {
        std::cout << "promise_type::final_suspend prev_ is empty" << std::endl;
      }
      return std::suspend_never{};
    }

    // GCOVR_EXCL_START
    // WASM 1.0 doesn't use exception
    inline void unhandled_exception() noexcept {
    }
    // GCOVR_EXCL_STOP

    inline void return_value(std::any value) noexcept {
      std::cout << "promise_type::return_value " << this << std::endl;
      if (value.has_value()) {
        std::cout << "promise_type::return_value owner is " << owner_ << std::endl;

        if (ready) {
          this->value_ = std::move(value);
        } else {
          owner_->value_ = value;
        }
      } else {
        std::cout << "promise_type::return_value value.has_value false" << std::endl;
      }
    }

    ~promise_type() {
      std::cout << "delete promise_type " << this << std::endl;
    }

  private:
    friend AwaitableReturnTask;
    std::coroutine_handle<promise_type> prev_;
    std::any value_;
    void set_owner(AwaitableReturnTask *owner) {
      std::cout << "bind promise " << this << " with AwaitableReturnTask " << owner << std::endl;
      owner_ = owner;
    }

    AwaitableReturnTask *owner_;
  };

  inline explicit AwaitableReturnTask(std::coroutine_handle<promise_type> handle) noexcept : handle_(handle) {
    std::cout << "new AwaitableReturnTask " << this << std::endl;
    handle_.promise().set_owner(this);
  }

  //  the three special methods that are called as part of a co_await
  inline bool await_ready() const noexcept {
    std::cout << "AwaitableReturnTask::await_ready " << this << std::endl;
    return ready; // suspend_always
  }

  inline void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
    // suspend coro and store coroutine state
    std::cout << "AwaitableReturnTask::await_suspend " << this << std::endl;
    handle_.promise().prev_ = h;
    // if return value is void or true, suspend sucessed. Controling right return to caller/resumer.
  }

  inline std::any await_resume() const noexcept {
    std::cout << "AwaitableReturnTask::await_resume " << this << std::endl;

    if (ready) {
      return handle_.promise().value_;
    } else {
      return value_;
    }
  }

  ~AwaitableReturnTask() {
    std::cout << "delete AwaitableReturnTask " << this << std::endl;
  }

private:
  std::coroutine_handle<promise_type> handle_; // !! To communicate with the promise_type
  std::any value_{};
};

template <typename T>
class Awaitable {

public:
  using RequestCallbackFunction = std::function<void(T)>;

  using RequestGenerateFunction = std::function<std::function<void()>(RequestCallbackFunction)>;

  explicit Awaitable(RequestGenerateFunction requestGenerateFunction) noexcept : requestGenerateFunction_(std::move(requestGenerateFunction)) {
  }

  inline bool await_ready() const noexcept {
    std::cout << "Awaitable::await_ready " << this << std::endl;
    return false;
  }

  void await_suspend(std::coroutine_handle<> const h) noexcept {
    std::cout << "Awaitable::await_suspend " << this << std::endl;
    h_ = h;
    auto const requestFunction = requestGenerateFunction_([this](T data) noexcept {
      asyncResume(std::move(data));
    });
    requestFunction();
  }

  inline T await_resume() noexcept {
    std::cout << "Awaitable::await_resume" << std::endl;
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

template <typename T>
class ImmAwaitable {

public:
  explicit ImmAwaitable(T data) noexcept : data_(std::move(data)) {
  }

  inline bool await_ready() const noexcept {
    std::cout << "ImmAwaitable::await_ready" << std::endl;
    return true;
  }

  void await_suspend(std::coroutine_handle<> const h) noexcept {
    static_cast<void>(h);
    assert(false);
  }

  inline T await_resume() noexcept {
    std::cout << "ImmAwaitable::await_resume" << std::endl;
    return std::move(data_);
  }

private:
  std::coroutine_handle<> h_;
  T data_;
};

using rawFunction = void (*)(uintptr_t);

std::queue<uintptr_t> taskQueue;

using CallbackPtr = void (*)();

void rawCallback(uintptr_t ctx) {
  std::function<void(uint8_t)> *ptr = reinterpret_cast<std::function<void(uint8_t)> *>(ctx);
  (*ptr)(1);
  delete ptr;
}

void rawMethod(uintptr_t ctx) {
  taskQueue.push(ctx);
}

Awaitable<int> callAsyncMethod() noexcept {
  auto requestGenerator = [](std::function<void(int)> resumeFunction) -> std::function<void()> {
    auto requestFunction = [resumeFunction = std::move(resumeFunction)]() {
      auto resumeWrapper = [resumeFunction] {
        resumeFunction(1);
      };

      std::function<void()> *fPtr = new std::function<void()>(resumeWrapper);

      rawMethod(reinterpret_cast<uintptr_t>(fPtr));
    };

    return requestFunction;
  };
  Awaitable<int> a(std::move(requestGenerator));
  return a;
}

AwaitableReturnTask goo1(int p) {
  std::cout << "enter goo1" << std::endl;
  std::any res;
  if (p > 0) {
    Awaitable<int> task = callAsyncMethod(); // call an async function
    res = co_await task;
  }

  int x = std::any_cast<int>(res);
  std::cout << "goo1 co_return" << std::endl;
  co_return x;
}

int loopTimes = 1;

AwaitableReturnTask goo2(int p) {
  std::cout << "enter goo2" << std::endl;
  for (int i = 0; i < loopTimes; i++) {
    std::any res = co_await goo1(p);
    std::cout << std::any_cast<int>(res) << std::endl;
  }
  std::cout << "goo2 co_return" << std::endl;
  co_return 0;
}
AwaitableReturnTask foo1() {
  int x = co_await ImmAwaitable<int>(5);
  std::cout << "foo1 co_await done" << std::endl;
  co_return x;
}

AwaitableReturnTask foo2() {
  for (int i = 0; i < loopTimes; i++) {
    std::any res = co_await foo1();
    std::cout << "foo2 co_await done" << std::endl;
    std::cout << std::any_cast<int>(res) << std::endl;
  }
  co_return 1;
}

AwaitableReturnTask yoo1(int in) {

  int x;
  if (in > 0) {
    Awaitable<int> task = callAsyncMethod(); // call an async function
    x = co_await task;
  } else {
    x = co_await ImmAwaitable<int>(5);
  }

  std::cout << "foo1 co_await done" << std::endl;
  co_return x;
}

AwaitableReturnTask yoo2(int in) {

  std::any res = co_await goo1(in);
  std::cout << std::any_cast<int>(res) << std::endl;
  co_return 1;
}

void runTimeMock() {

  while (!taskQueue.empty()) {
    uintptr_t ctx = taskQueue.front();

    rawCallback(ctx);
    taskQueue.pop();
  }
}

int main() {
  // ready = true;
  // foo2();

  ready = false;
  goo2(1);

  // ready = true;
  // yoo1(-1);

  // ready = false;
  // yoo2(1);

  runTimeMock();
  std::cout << "DOne" << std::endl;
}