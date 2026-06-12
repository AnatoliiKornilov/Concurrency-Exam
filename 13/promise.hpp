#pragma once

#include <exception>

#include "state.hpp"

namespace future {

////////////////////////////////////////////////////////////////////////////////
// Exceptions
////////////////////////////////////////////////////////////////////////////////

class PromiseException : public std::exception {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

class PromiseInvalid : public PromiseException {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

class PromiseAlreadySatisfied : public PromiseException {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

class PromiseBroken : public PromiseException {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

////////////////////////////////////////////////////////////////////////////////
// Promise
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Future;

template <typename T>
class Promise {
public:
    Promise() = default;

    // Not copyable.
    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;

    // Movable.
    Promise(Promise&& other) noexcept;
    Promise& operator=(Promise&&) noexcept;

    /// Postconditions:
    /// - If `IsValid()` and `!IsFulfilled()`, the associated future (if any) will be completed with
    ///   a `PromiseBroken` exception *as if* by `SetError(...)`.
    /// - If `IsValid()`, releases, possibly destroying, the shared state.
    ~Promise();

    ////////////////////////////////////////////////////////////////////////////

    /// The following methods fulfill the Promise with the specified value.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws PromiseInvalid)
    /// - `IsFulfilled() == false` (else throws PromiseAlreadySatisfied)
    ///
    /// Postconditions:
    /// - `IsFulfilled() == true`
    /// - `IsValid() == true` (unchanged)
    /// - The associated future will see the value, e.g., in its continuation.
    template <typename M>
    void SetValue(M&& value) &&;
    void SetError(std::exception_ptr exception) &&;

    ////////////////////////////////////////////////////////////////////////////

    /// - True if this has a shared state;
    /// - False if this has been moved-out.
    bool IsValid() const noexcept;

    /// - True if `!IsValid()` or `IsValid()` and  was fulfilled (a prior call to `Set...()`;
    /// - False otherwise.
    bool IsFulfilled() const noexcept;

private:
    template <typename Q>
    friend std::pair<Future<Q>, Promise<Q>> GetTied();

    detail::SharedState<T>* state_ = nullptr;

private:
    Promise(detail::SharedState<T>* state);
};

}  // namespace future

#define PROMISE_IMPL
#include "promise.ipp"
#undef PROMISE_IMPL
