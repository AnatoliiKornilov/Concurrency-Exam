#pragma once

#include "future.hpp"

#include <type_traits>
#include <tuple>

namespace future {

/// When all input Futures have completed, the returned Future will also complete. Errors do not
/// cause early termination, ensuring that this Future always succeeds after all its constituent
/// Futures have finished — whether successfully or with errors.
template <typename... Fs>
Future<std::tuple<Result<typename std::remove_cvref_t<Fs>::ValueType>...>> CollectAll(Fs... fs);

/// Similar to CollectAll, but short-circuits at the first exception encountered. Therefore, the
/// type of the returned Future is Future<std::tuple<T1, T2, ...>> instead of
/// Future<std::tuple<Result<T1>, Result<T2>, ...>>.
template <typename... Fs>
Future<std::tuple<typename std::remove_cvref_t<Fs>::ValueType...>> Collect(Fs... fs);

/// The result is a variant containing the first Future to complete. If multiple Futures complete
/// simultaneously (or are already complete when passed in), the "winner" is chosen
/// non-deterministically.
template <typename... Fs>
Future<std::variant<Result<typename std::remove_cvref_t<Fs>::ValueType>...>> CollectAny(Fs... fs);

/// Similar to CollectAny, CollectAnyWithoutException returns the first Future to complete without
/// exceptions. If all Futures complete with exceptions, the last exception encountered will be
/// returned as the result.
template <typename... Fs>
Future<std::variant<typename std::remove_cvref_t<Fs>::ValueType...>> CollectAnyWithoutException(
    Fs... fs);

}  // namespace future

#define COMBINE_IMPL
#include "combine.ipp"
#undef COMBINE_IMPL
