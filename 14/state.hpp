#pragma once

#include <variant>
#include <exception>
#include <utility>
#include <optional>

#include <lines/std/atomic.hpp>
#include <lines/std/condvar.hpp>
#include <lines/std/mutex.hpp>

namespace future {

using Error = std::exception_ptr;

namespace detail {

template <typename T>
class SharedState {
private:
    struct Container {
        std::optional<T> value;
        Error error = nullptr;
    };

public:
    SharedState() = default;

    void SetValue(T value) {
        mutex_.lock();

        container_.value.emplace(std::move(value));

        mutex_.unlock();

        cv_.NotifyOne();
    }

    void SetError(Error error) {
        mutex_.lock();

        container_.error = error;

        mutex_.unlock();

        cv_.NotifyOne();
    }

    T Get() {
        mutex_.lock();

        while (!container_.value.has_value() && container_.error == nullptr) {
            cv_.Wait(mutex_);
        }

        if (container_.error != nullptr) {
            mutex_.unlock();
            std::rethrow_exception(container_.error);
        }

        mutex_.unlock();

        return std::move(*container_.value);
    }

private:
    Container container_;

    lines::Mutex mutex_;

    lines::Condvar cv_;
};

}  // namespace detail

}  // namespace future
