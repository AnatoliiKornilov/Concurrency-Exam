#ifndef FUTURE_IMPL
#include "future.hpp"
#error Do not include this file directly
#endif

namespace future {

template <typename T>
Future<T>::Future(detail::SharedState<T>* state) : state_(state) {
    state_->AddRef();
}

template <typename T>
Future<T>::Future(Future&& other) noexcept : state_(other.state_) {
    other.state_ = nullptr;
}

template <typename T>
Future<T>& Future<T>::operator=(Future&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    state_ = other.state_;
    other.state_ = nullptr;

    return *this;
}

template <typename T>
Future<T>::~Future() {
    if (state_ != nullptr) {
        state_->Detach();
    }
}

template <typename T>
template <typename Self>
auto&& Future<T>::GetResult(this Self&& self) {
    if (!self.IsValid()) {
        throw FutureInvalid();
    }

    if (!self.IsReady()) {
        throw FutureNotReady();
    }

    return std::forward_like<Self>(self.state_->GetResult());
}

template <typename T>
template <typename Self>
auto&& Future<T>::GetValue(this Self&& self) {
    auto&& result = std::forward_like<Self>(self).GetResult();

    if (!result.has_value()) {
        std::rethrow_exception(result.error());
    }

    return std::forward_like<Self>(result.value());
}

template <typename T>
Future<T>& Future<T>::Wait() & {
    if (!IsValid()) {
        throw FutureInvalid();
    }

    while (!IsReady()) {
    }

    return *this;
}

template <typename T>
Future<T>&& Future<T>::Wait() && {
    (*this).Wait();

    return std::move(*this);
}

template <typename T>
bool Future<T>::IsValid() const noexcept {
    return state_ != nullptr;
}

template <typename T>
bool Future<T>::IsReady() const {
    if (!IsValid()) {
        throw FutureInvalid();
    }

    return state_->HasResult();
}

template <typename T>
bool Future<T>::HasException() const {
    if (!IsValid()) {
        throw FutureInvalid();
    }

    if (!IsReady()) {
        throw FutureNotReady();
    }

    return !(state_->GetResult().has_value());
}

template <typename T>
template <detail::SubscribeCallback<Result<T>> F>
void Future<T>::Subscribe(F&& callback) && {
    if (!IsValid()) {
        throw FutureInvalid();
    }

    state_->SetCallback(std::forward<F>(callback));

    state_->Detach();

    state_ = nullptr;
}

template <class T>
[[nodiscard]] std::pair<Future<T>, Promise<T>> GetTied() {
    detail::SharedState<T>* state = new detail::SharedState<T>();

    Promise<T> promise(state);
    Future<T> future(state);

    return std::pair<Future<T>, Promise<T>>(std::move(future), std::move(promise));
}

template <typename T>
template <detail::ThenSync<T> F>
Future<std::invoke_result_t<F, T>> Future<T>::ThenValue(F&& func) && {
    if (!IsValid()) {
        throw FutureInvalid();
    }

    using ResType = std::invoke_result_t<F, T>;

    auto [future_res, promise_res] = GetTied<ResType>();

    auto lambda_callback = [function = std::forward<F>(func),
                            promise = std::move(promise_res)](Result<T>&& result) mutable noexcept {
        if (!result.has_value()) {
            std::move(promise).SetError(result.error());
            return;
        }

        try {
            std::move(promise).SetValue(function(std::move(result).value()));
        } catch (...) {
            std::move(promise).SetError(std::current_exception());
        }
    };

    std::move(*this).Subscribe(std::move(lambda_callback));

    return std::move(future_res);
}

template <typename T>
template <detail::ThenAsync<T> F>
Future<typename std::invoke_result_t<F, T>::ValueType> Future<T>::ThenValue(F&& func) && {
    if (!IsValid()) {
        throw FutureInvalid();
    }

    using InnerFuture = std::invoke_result_t<F, T>;
    using ResType = typename InnerFuture::ValueType;

    auto [future_res, promise_res] = GetTied<ResType>();

    auto lambda_callback = [function = std::forward<F>(func),
                            promise = std::move(promise_res)](Result<T>&& result) mutable noexcept {
        if (!result.has_value()) {
            std::move(promise).SetError(result.error());
            return;
        }

        try {
            auto inner_future = function(std::move(result).value());

            if (!inner_future.IsValid()) {
                std::move(promise).SetError(std::make_exception_ptr(FutureInvalid()));

            } else {

                std::move(inner_future)
                    .Subscribe([inner_promise = std::move(promise)](
                                   Result<ResType>&& inner_result) mutable noexcept {
                        if (!inner_result.has_value()) {
                            std::move(inner_promise).SetError(inner_result.error());
                            return;
                        }

                        try {
                            std::move(inner_promise).SetValue(std::move(inner_result).value());
                        } catch (...) {
                            std::move(inner_promise).SetError(std::current_exception());
                        }
                    });
            }

        } catch (...) {
            std::move(promise).SetError(std::current_exception());
            return;
        }
    };

    std::move(*this).Subscribe(std::move(lambda_callback));

    return std::move(future_res);
}

template <typename T>
template <detail::ThenSync<std::exception_ptr> F>
Future<T> Future<T>::ThenError(F&& func) && {
    if (!IsValid()) {
        throw FutureInvalid();
    }

    using ResType = T;

    auto [future_res, promise_res] = GetTied<ResType>();

    auto lambda_callback = [function = std::forward<F>(func),
                            promise = std::move(promise_res)](Result<T>&& result) mutable noexcept {
        if (result.has_value()) {
            std::move(promise).SetValue(std::move(result).value());
            return;
        }

        try {
            std::move(promise).SetValue(function(result.error()));
        } catch (...) {
            std::move(promise).SetError(std::current_exception());
        }
    };

    std::move(*this).Subscribe(std::move(lambda_callback));

    return std::move(future_res);
}

template <typename T>
template <detail::ThenAsync<std::exception_ptr> F>
Future<T> Future<T>::ThenError(F&& func) && {
    if (!IsValid()) {
        throw FutureInvalid();
    }

    using InnerFuture = Future<T>;
    using ResType = typename InnerFuture::ValueType;

    auto [future_res, promise_res] = GetTied<ResType>();

    auto lambda_callback = [function = std::forward<F>(func),
                            promise = std::move(promise_res)](Result<T>&& result) mutable noexcept {
        if (result.has_value()) {
            std::move(promise).SetValue(std::move(result).value());
            return;
        }

        try {
            auto inner_future = function(result.error());

            if (!inner_future.IsValid()) {
                std::move(promise).SetError(std::make_exception_ptr(FutureInvalid()));

            } else {

                std::move(inner_future)
                    .Subscribe([inner_promise = std::move(promise)](
                                   Result<ResType>&& inner_result) mutable noexcept {
                        if (inner_result.has_value()) {
                            std::move(inner_promise).SetValue(std::move(inner_result).value());
                            return;
                        }

                        try {
                            std::move(inner_promise).SetError(inner_result.error());
                        } catch (...) {
                            std::move(inner_promise).SetError(std::current_exception());
                        }
                    });
            }

        } catch (...) {
            std::move(promise).SetError(std::current_exception());
            return;
        }
    };

    std::move(*this).Subscribe(std::move(lambda_callback));

    return std::move(future_res);
}

}  // namespace future
