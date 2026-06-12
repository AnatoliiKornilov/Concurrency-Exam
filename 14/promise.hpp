#pragma once

#include <memory>

#include "state.hpp"

namespace future {

template <typename T>
class Future;

template <typename T>
class Promise {
public:
    Promise() = default;
    Promise(const Promise&) = delete;
    Promise(Promise&& other) = default;

    void SetValue(T value) && {
        state_->SetValue(std::move(value));
    }

    void SetError(Error error) && {
        state_->SetError(error);
    }

private:
    friend class Future<T>;

    std::shared_ptr<detail::SharedState<T>> state_ = std::make_shared<detail::SharedState<T>>();
};

}  // namespace future
