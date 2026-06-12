#pragma once

#include <coroutine>
#include <future>
#include <variant>

namespace coro {

////////////////////////////////////////////////////////////////////////////////
// Exceptions
////////////////////////////////////////////////////////////////////////////////

class AsyncTaskInvalid : public std::exception {
public:
    const char* what() const noexcept override;
};

////////////////////////////////////////////////////////////////////////////////
// Class declarations
////////////////////////////////////////////////////////////////////////////////

// Don't bother with void types, just use Unit.
using Unit = std::monostate;

template <typename T>
class AsyncTask;

namespace detail {

template <typename T>
class AsyncTaskAwaiter;

template <typename T>
class AsyncTaskPromise;

class FinalSuspendAwaitable;

}  // namespace detail

////////////////////////////////////////////////////////////////////////////////
// Methods declarations
////////////////////////////////////////////////////////////////////////////////

/// Represents an allocated, but not-started coroutine.
template <typename T>
class AsyncTask {
public:
    /// Define promise type for compiler.
    using promise_type = detail::AsyncTaskPromise<T>;

    /// Default-constructable.
    ///
    /// Postconditions:
    /// - `IsValid() == false`
    AsyncTask() noexcept = default;

    /// Not copyable.
    AsyncTask(const AsyncTask&) = delete;
    AsyncTask& operator=(const AsyncTask&) = delete;

    /// Movable.
    AsyncTask(AsyncTask<T>&&) noexcept;
    AsyncTask<T>& operator=(AsyncTask<T>&&) noexcept;

    /// Checks whether this task holds coroutine.
    bool IsValid() const noexcept;

    /// Starts the coroutine, invalidates the current task and returns
    /// the future that is set when the coroutine is completed.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws AsyncTaskInvalid)
    ///
    /// Postconditions:
    /// - `IsValid() == false`
    std::future<T> Run() &&;

    /// This method allows awaiting the current task.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws AsyncTaskInvalid)
    ///
    /// Postconditions:
    /// - `IsValid() == false`
    detail::AsyncTaskAwaiter<T> operator co_await() &&;

    // If the coroutine is not started, it should be destroyed.
    ~AsyncTask() noexcept;

private:
    // Users should not be able to call this constructor explicitly,
    // it is the details of implementation.
    AsyncTask(std::coroutine_handle<promise_type>) noexcept;

    // You may need to declare friend classes here.

    std::coroutine_handle<promise_type> handle_;

    friend promise_type;
};

////////////////////////////////////////////////////////////////////////////////

namespace detail {

/// Promise type for AsyncTask coroutines.
///
/// When a corotuine (function that has key-words co_await or co_return) returns
/// AsyncTask, compiler gets the AsyncTask::promise_type and creates coroutine
/// as well as promise on the heap. So the lifetime of promise is bounded to the
/// lifetime of the coroutine.
template <typename T>
class AsyncTaskPromise {
public:
    // Define the following methods:
    //
    //  - get_return_object() - Called after the coroutine and promise is
    //    created to obtain the return object
    //
    //  - initial_suspend() - Defines the behaviour of just started coroutine.
    //    As we implement lazily-started coroutines, we need to supsend
    //    the coroutine just after it is created.
    //
    //  - return_value(...) - Called when `co_return expr` is called. You may
    //    store the result inside this method.
    //
    //  - unhandled_exception() - This method is called if the coroutine ends
    //    with an uncaught exception.
    //
    //  - final_suspend() - Defines the behaviour of coroutine that has just
    //    finished it's execution
    //
    // Do not forget about `noexcept` specifier!

    AsyncTask<T> get_return_object();

    std::suspend_always initial_suspend();

    void return_value(T);

    void unhandled_exception();

    FinalSuspendAwaitable final_suspend() noexcept;

private:
    // It is a good place to store the coroutine result, as well as other
    // objects that should be bound to the lifetime of the coroutine
    // (e.g. executor, cancellation token, continuation, etc.)
    // Also, you may need to declare friend classes here.
    //
    // Try not to wrap std::coroutine_handle into std::function here to safe
    // overhead costs and make the code more readable.

    std::coroutine_handle<> continuation_ = nullptr;

    std::promise<T> promise_;

    friend class AsyncTask<T>;
    friend class FinalSuspendAwaitable;
    friend class AsyncTaskAwaiter<T>;
};

////////////////////////////////////////////////////////////////////////////////

/// This awaitable is used to define the behaviour of the coroutine that has
/// finished it's execution. You may find this custimization point useful to
/// shedule the execution of the continuation.
class FinalSuspendAwaitable {
public:
    // Define the following methods:
    //  - bool await_ready() noexcept
    //  - ... await_suspend(...) noexcept
    //  - void await_resume() noexcept
    //
    // Do not forget about `noexcept` specifier!
    // If you implemented everything correctly, await_resume should never be
    // called. You may use `UNREACHABLE()` to abort execution in that method.

    // TODO: Your solution

    inline bool await_ready() noexcept;

    template <typename PromiseType>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseType>) noexcept;

    inline void await_resume() noexcept;
};

////////////////////////////////////////////////////////////////////////////////

/// This awaiter is used to await the completion of the AsyncTask.
/// You may use this awaiter to:
///  1. Set the continuation.
///  2. Start the coroutine.
/// Note that if you implemented everything correctly, you will not need any
/// synchronization primitives, because the task is lazily-started.
template <typename T>
class AsyncTaskAwaiter {
public:
    // Define the following methods:
    //  - bool await_ready()
    //  - ... await_suspend(...)
    //  - T await_resume()

    AsyncTaskAwaiter(std::coroutine_handle<AsyncTaskPromise<T>>);

    bool await_ready();

    std::coroutine_handle<> await_suspend(std::coroutine_handle<>);

    T await_resume();

private:
    std::coroutine_handle<AsyncTaskPromise<T>> target_;
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace detail

}  // namespace coro

#define PROMISE_IMPL
#include "async_task.ipp"
#undef PROMISE_IMPL
