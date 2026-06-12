#pragma once

#include <utility>

#include "promise.hpp"
#include "state.hpp"

namespace future {

template <typename T>
class Future {
public:
    Future(Promise<T>& promise) : state_(promise.state_) {
    }

    Future(const Future&) = delete;
    Future(Future&&) = default;

    T Get() && {
        return state_->Get();
    }

private:
    std::shared_ptr<detail::SharedState<T>> state_;
};

template <class T>
std::pair<Future<T>, Promise<T>> GetTied() {
    Promise<T> p;
    Future<T> f(p);

    return std::make_pair(std::move(f), std::move(p));
}

}  // namespace future
