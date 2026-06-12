#ifndef PROMISE_IMPL
#include "promise.hpp"
#error Do not include this file directly
#endif

namespace future {

template <typename T>
Promise<T>::Promise(detail::SharedState<T>* state) : state_(state) {
    state_->AddRef();
}

template <typename T>
Promise<T>::Promise(Promise<T>&& other) noexcept : state_(other.state_) {
    other.state_ = nullptr;
}

template <typename T>
Promise<T>& Promise<T>::operator=(Promise<T>&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    state_ = other.state_;
    other.state_ = nullptr;

    return *this;
}

template <typename T>
Promise<T>::~Promise() {
    if (!IsValid()) {
        return;
    }

    if (!IsFulfilled()) {
        state_->SetResult(std::unexpected(std::make_exception_ptr(PromiseBroken())));
    }

    state_->Detach();
}

template <typename T>
template <typename M>
void Promise<T>::SetValue(M&& value) && {
    if (!IsValid()) {
        throw PromiseInvalid();
    }

    if (IsFulfilled()) {
        throw PromiseAlreadySatisfied();
    }

    state_->SetResult(Result<T>(std::forward<M>(value)));
}

template <typename T>
void Promise<T>::SetError(std::exception_ptr exception) && {
    if (!IsValid()) {
        throw PromiseInvalid();
    }

    if (IsFulfilled()) {
        throw PromiseAlreadySatisfied();
    }

    state_->SetResult(Result<T>(std::unexpect, std::move(exception)));
}

template <typename T>
bool Promise<T>::IsValid() const noexcept {
    return state_ != nullptr;
}

template <typename T>
bool Promise<T>::IsFulfilled() const noexcept {
    return !IsValid() || state_->HasResult();
}

}  // namespace future
