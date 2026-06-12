#pragma once

#include <utility>
#include <type_traits>

#include "promise.hpp"
#include "state.hpp"
#include "meta.hpp"

namespace future {

////////////////////////////////////////////////////////////////////////////////
// Exceptions
////////////////////////////////////////////////////////////////////////////////

class FutureException : public std::exception {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

class FutureInvalid : public FutureException {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

class FutureNotReady : public FutureException {
public:
    // NOLINTNEXTLINE
    const char* what() const noexcept override;
};

////////////////////////////////////////////////////////////////////////////////
// Future
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Promise;

template <typename T>
class Future {
public:
    using ValueType = T;

public:
    Future() = default;

    // Not copyable.
    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    // Movable.
    Future(Future&& other) noexcept;
    Future& operator=(Future&&) noexcept;

    ~Future();

    ////////////////////////////////////////////////////////////////////////////

    /// Returns a reference to the result if it is ready.
    ///
    /// Does not `Wait()`.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws FutureInvalid)
    /// - `IsReady() == true` (else throws FutureNotReady)
    ///
    /// Postconditions:
    /// - This call does not mutate the value of the future. However, the calling code may mutate
    ///   that value, including moving it out by move-constructing or move-assigning another value
    ///   from it, for example, via the & or && overloads or through casts.
    template <typename Self>
    auto&& GetResult(this Self&& self);

    /// Returns a reference to the result value if it is ready.
    ///
    /// Does not `Wait()`.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws FutureInvalid)
    /// - `IsReady() == true` (else throws FutureNotReady)
    ///
    /// Postconditions:
    /// - If an exception has been captured (i.e., if `HasException() == true`), throws that
    ///   exception.
    /// - This call does not mutate the value of the future. However, the calling code may mutate
    ///   that value, including moving it out by move-constructing or move-assigning another value
    ///   from it, for example, via the & or && overloads or through casts.
    template <typename Self>
    auto&& GetValue(this Self&& self);

    ////////////////////////////////////////////////////////////////////////////

    /// Blocks until this Future is complete.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws FutureInvalid)
    ///
    /// Postconditions:
    /// - `IsValid() == true` (but the calling code can trivially move-out `*this` by assigning or
    ///   constructing the result into a distinct object).
    /// - `&RESULT == this`
    /// - `IsReady() == true`
    Future<T>& Wait() &;
    Future<T>&& Wait() &&;

    ////////////////////////////////////////////////////////////////////////////

    /// - True if this has a shared state;
    /// - False if this has been moved-out.
    bool IsValid() const noexcept;

    /// True when the result (or exception) is ready.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws FutureInvalid)
    bool IsReady() const;

    /// True if the result of a Future is an exception (not a value) and IsReady() returns true.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws FutureInvalid)
    /// - `IsReady() == true` (else throws FutureNotReady)
    bool HasException() const;

    ////////////////////////////////////////////////////////////////////////////

    /// Once this Future has completed, execute func, a function that takes T&& and returns either a
    /// V or a Future<V>.
    ///
    /// Example:
    ///   Future<string> f2 = f1.ThenValue([](auto&& v) {
    ///     ...
    ///     return string("foo");
    ///   });
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws FutureInvalid)
    ///
    /// Postconditions:
    /// - `IsValid() == false`
    /// - `RESULT.IsValid() == true`
    template <detail::ThenSync<T> F>
    Future<std::invoke_result_t<F, T>> ThenValue(F&& func) &&;

    template <detail::ThenAsync<T> F>
    Future<typename std::invoke_result_t<F, T>::ValueType> ThenValue(F&& func) &&;

    /// Set an error continuation for this Future such that the continuation is invoked with an
    /// exception type and returns either a T or a Future<T>.
    ///
    /// Example:
    ///   Future<string> f2 = f1.ThenValue([] {
    ///       throw std::runtime_error("oh no!");
    ///       return "42";
    ///     })
    ///     .ThenError([] (std::exception& e) -> std::string {
    ///       return e.what();
    ///     });
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws FutureInvalid)
    ///
    /// Postconditions:
    /// - `IsValid() == false`
    /// - `RESULT.IsValid() == true`
    template <detail::ThenSync<std::exception_ptr> F>
    Future<T> ThenError(F&& func) &&;

    template <detail::ThenAsync<std::exception_ptr> F>
    Future<T> ThenError(F&& func) &&;

    ////////////////////////////////////////////////////////////////////////////

    /// When this Future has completed, execute func, a function that takes Result<T>&& and returns
    /// void. The func callback must not throw any exceptions. This method is designed for internal
    /// use and should not be utilized directly by users. Instead, users are encouraged to employ
    /// futures chaining and combinators for handling Future objects.
    ///
    /// Preconditions:
    /// - `IsValid() == true` (else throws FutureInvalid)
    ///
    /// Postconditions:
    /// - `IsValid() == false`
    template <detail::SubscribeCallback<Result<T>> F>
    void Subscribe(F&& callback) &&;

private:
    template <class Q>
    friend std::pair<Future<Q>, Promise<Q>> GetTied();

    template <class U>
    friend class Future;

    detail::SharedState<T>* state_ = nullptr;

private:
    Future(detail::SharedState<T>* state);
};

////////////////////////////////////////////////////////////////////////////////

template <class T>
[[nodiscard]] std::pair<Future<T>, Promise<T>> GetTied();

}  // namespace future

#define FUTURE_IMPL
#include "future.ipp"
#undef FUTURE_IMPL
