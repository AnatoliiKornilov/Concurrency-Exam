#pragma once

#include <expected>

#include <lines/std/atomic.hpp>
#include <function2/function2.hpp>

namespace future {

template <typename T>
using Result = std::expected<T, std::exception_ptr>;

struct Unit {};

namespace detail {

template <typename T>
class SharedState {
public:
    using Callback = fu2::unique_function<void(Result<T>&& result) noexcept>;

public:
    SharedState();
    ~SharedState();

    void SetResult(Result<T>&& result);
    void SetCallback(Callback&& callback);

    bool HasResult() const noexcept;
    bool HasCallback() const noexcept;

    /// Called by a destructing Future and Promise.
    /// Calls `delete this` if there are no more references to `this`.
    void Detach() noexcept;

    void AddRef() noexcept;

    // Precondition: HasResult() == true;
    template <typename Self>
    auto&& GetResult(this Self&& self) noexcept;

private:
    enum class State {
        Empty,
        OnlyResult,
        OnlyCallback,
        Done,
    };

private:
    lines::Atomic<State> state_ = State::Empty;

    lines::Atomic<unsigned int> ref_cnt_ = 0;

    Result<T> value_or_error_ = std::unexpected(nullptr);

    Callback callback_ = nullptr;
};

}  // namespace detail
}  // namespace future

#define PROMISE_IMPL
#include "state.ipp"
#undef PROMISE_IMPL
