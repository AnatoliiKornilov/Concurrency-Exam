#pragma once

#include <concepts>
#include <utility>

namespace future {

template <class T>
class Future;

namespace detail {

template <class T>
concept IsFuture = requires(T& value) {
    { Future(std::move(value)) } -> std::same_as<T>;
};

template <class F, class T>
concept ThenAsync = requires(F& func, T& value) {
    { func(std::move(value)) } -> IsFuture;
};

template <class F, class T>
concept ThenSync = !ThenAsync<F, T> && requires(F& func, T& value) {
    { func(std::move(value)) };
};

template <class F, class T>
concept SubscribeCallback = requires(F& func, T& value) {
    { func(std::move(value)) } noexcept -> std::same_as<void>;
};

}  // namespace detail

}  // namespace future
