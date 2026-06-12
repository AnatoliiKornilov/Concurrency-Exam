#ifndef PROMISE_IMPL
#include "state.hpp"
#error Do not include this file directly
#endif

namespace future::detail {

template <typename T>
SharedState<T>::SharedState() = default;

template <typename T>
SharedState<T>::~SharedState() = default;

template <typename T>
void SharedState<T>::SetResult(Result<T>&& result) {
    State expected = State::Empty;

    value_or_error_ = std::forward<Result<T>>(result);

    if (!state_.compare_exchange_strong(expected, State::OnlyResult)) {
        state_.store(State::Done);

        callback_(std::move(value_or_error_));
    }
}

template <typename T>
void SharedState<T>::SetCallback(Callback&& callback) {
    State expected = State::Empty;

    callback_ = std::forward<Callback>(callback);

    if (!state_.compare_exchange_strong(expected, State::OnlyCallback)) {
        state_.store(State::Done);

        callback_(std::move(value_or_error_));
    }
}

template <typename T>
bool SharedState<T>::HasResult() const noexcept {
    return state_ == State::OnlyResult || state_ == State::Done;
}

template <typename T>
bool SharedState<T>::HasCallback() const noexcept {
    return state_ == State::OnlyCallback || state_ == State::Done;
}

template <typename T>
void SharedState<T>::Detach() noexcept {
    if (ref_cnt_ > 1) {
        --ref_cnt_;

        return;
    }

    delete this;
}

template <typename T>
void SharedState<T>::AddRef() noexcept {
    ++ref_cnt_;
}

template <typename T>
template <typename Self>
auto&& SharedState<T>::GetResult(this Self&& self) noexcept {
    return std::forward_like<Self>(self.value_or_error_);
}

}  // namespace future::detail
