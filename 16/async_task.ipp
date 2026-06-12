#ifndef PROMISE_IMPL
#include "async_task.hpp"
#error Do not include this file directly
#endif

#include <libassert/assert.hpp>

// TODO: Your solution

namespace coro {

namespace detail {

// AsyncTaskPromise

template <typename T>
AsyncTask<T> AsyncTaskPromise<T>::get_return_object() {
    return AsyncTask<T>{std::coroutine_handle<AsyncTaskPromise<T>>::from_promise(*this)};
}

template <typename T>
std::suspend_always AsyncTaskPromise<T>::initial_suspend() {
    return std::suspend_always{};
}

template <typename T>
void AsyncTaskPromise<T>::return_value(T value) {
    promise_.set_value(std::move(value));
}

template <typename T>
void AsyncTaskPromise<T>::unhandled_exception() {
    promise_.set_exception(std::current_exception());
}

template <typename T>
FinalSuspendAwaitable AsyncTaskPromise<T>::final_suspend() noexcept {
    return FinalSuspendAwaitable{};
}

// FinalSuspendAwaitable

inline bool FinalSuspendAwaitable::await_ready() noexcept {
    return false;
}

template <typename PromiseType>
std::coroutine_handle<> FinalSuspendAwaitable::await_suspend(
    std::coroutine_handle<PromiseType> handle) noexcept {
    std::coroutine_handle<> next = handle.promise().continuation_;

    return next != nullptr ? next : std::noop_coroutine();
}

inline void FinalSuspendAwaitable::await_resume() noexcept {
    UNREACHABLE();
}

// AsyncTaskAwaiter

template <typename T>
AsyncTaskAwaiter<T>::AsyncTaskAwaiter(std::coroutine_handle<AsyncTaskPromise<T>> target)
    : target_(target) {
}

template <typename T>
bool AsyncTaskAwaiter<T>::await_ready() {
    return false;
}

template <typename T>
std::coroutine_handle<> AsyncTaskAwaiter<T>::await_suspend(std::coroutine_handle<> waiting) {
    target_.promise().continuation_ = waiting;

    return target_;
}

template <typename T>
T AsyncTaskAwaiter<T>::await_resume() {
    std::future<T> future = target_.promise().promise_.get_future();

    try {
        T result = future.get();

        target_.destroy();

        return result;
    } catch (...) {
        target_.destroy();

        std::rethrow_exception(std::current_exception());
    }
}

}  // namespace detail

// AsyncTask

template <typename T>
AsyncTask<T>::AsyncTask(std::coroutine_handle<promise_type> handle) noexcept : handle_(handle) {
}

template <typename T>
AsyncTask<T>::AsyncTask(AsyncTask<T>&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)) {
}

template <typename T>
AsyncTask<T>& AsyncTask<T>::operator=(AsyncTask<T>&& other) noexcept {
    if (this != &other) {
        if (handle_ != nullptr) {
            handle_.destroy();
        }

        handle_ = std::exchange(other.handle_, nullptr);
    }

    return *this;
}

template <typename T>
bool AsyncTask<T>::IsValid() const noexcept {
    return handle_ != nullptr;
}

template <typename T>
std::future<T> AsyncTask<T>::Run() && {
    if (!IsValid()) {
        throw AsyncTaskInvalid{};
    }
    std::promise<T> promise;
    std::future<T> future = promise.get_future();

    handle_.promise().promise_ = std::move(promise);

    handle_.resume();

    if (handle_.done()) {
        handle_.destroy();
    }

    handle_ = nullptr;

    return std::move(future);
}

template <typename T>
detail::AsyncTaskAwaiter<T> AsyncTask<T>::operator co_await() && {
    if (!IsValid()) {
        throw AsyncTaskInvalid{};
    }

    std::coroutine_handle<promise_type> handle = std::exchange(handle_, nullptr);

    return detail::AsyncTaskAwaiter<T>{handle};
}

template <typename T>
AsyncTask<T>::~AsyncTask() noexcept {
    if (handle_ != nullptr) {
        handle_.destroy();
    }
}

}  // namespace coro
