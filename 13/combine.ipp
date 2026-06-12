#ifndef COMBINE_IMPL
#include "combine.hpp"
#error Do not include this file directly
#endif

// TODO: Your solution

namespace future {

namespace detail {

template <size_t Ind, typename Func, typename Head>
void ForEach(Func&& func, Head&& head) {
    func(std::integral_constant<size_t, Ind>{}, head);
}

template <size_t Ind, typename Func, typename Head, typename... Tail>
void ForEach(Func&& func, Head&& head, Tail&&... tail) {
    func(std::integral_constant<size_t, Ind>{}, head);
    ForEach<Ind + 1>(func, tail...);
}

// You may find this function useful.
// Usage:
//
// ForEach([&](auto index, auto&& future) {
//     ...
//     std::get<index.value>(tuple) = ...;
//     ...
// }, fs...);
template <typename V, typename... Fs>
void ForEach(V&& func, Fs&&... fs) {
    ForEach<0>(static_cast<V&&>(func), static_cast<Fs&&>(fs)...);
}

template <typename... Ts>
class DataForAll {
public:
    DataForAll(Promise<std::tuple<Result<Ts>...>>&& promise) : promise_(std::move(promise)) {
    }

    ~DataForAll() = default;

    template <size_t Index>
    void AddResult(Result<typename std::tuple_element<Index, std::tuple<Ts...>>::type>&& result) {
        std::get<Index>(data_) = std::move(result);

        if (++value_cnt_ == std::tuple_size_v<decltype(data_)>) {
            std::tuple<Result<Ts>...> ans = std::apply(
                [](auto&... opts) {
                    return std::tuple<Result<Ts>...>{std::move(*opts)...};
                },
                data_);

            std::move(promise_).SetValue(std::move(ans));

            delete this;
        }
    }

private:
    std::tuple<std::optional<Result<Ts>>...> data_;

    Promise<std::tuple<Result<Ts>...>> promise_;

    lines::Atomic<size_t> value_cnt_ = 0;
};

template <typename... Ts>
class Data {
public:
    Data(Promise<std::tuple<Ts...>>&& promise) : promise_(std::move(promise)) {
    }

    ~Data() = default;

    template <size_t Index>
    void AddValueOrSetError(
        Result<typename std::tuple_element<Index, std::tuple<Ts...>>::type>&& result) {

        if (!done_.load()) {
            if (!result.has_value()) {
                done_.store(true);

                std::move(promise_).SetError(result.error());
            } else {
                std::get<Index>(data_) = std::move(result).value();
            }
        }

        bool expected = false;
        if (++value_cnt_ == std::tuple_size_v<decltype(data_)>) {
            if (done_.compare_exchange_strong(expected, true)) {

                std::tuple<Ts...> ans = std::apply(
                    [](auto&... opts) {
                        return std::tuple<Ts...>{std::move(*opts)...};
                    },
                    data_);

                std::move(promise_).SetValue(std::move(ans));
            }

            delete this;
        }
    }

private:
    std::tuple<std::optional<Ts>...> data_;

    Promise<std::tuple<Ts...>> promise_;

    lines::Atomic<size_t> value_cnt_ = 0;

    lines::Atomic<bool> done_ = false;
};

template <typename... Ts>
class DataForAny {
public:
    DataForAny(Promise<std::variant<Result<Ts>...>>&& promise, size_t future_cnt)
        : promise_(std::move(promise)), last_(future_cnt) {
    }

    ~DataForAny() = default;

    template <size_t Index>
    void SetResult(Result<typename std::tuple_element<Index, std::tuple<Ts...>>::type>&& result) {

        bool expected = false;
        if (done_.compare_exchange_strong(expected, true)) {

            std::move(promise_).SetValue(
                std::variant<Result<Ts>...>{std::in_place_index<Index>, std::move(result)});
        }

        if (--last_ == 0) {
            delete this;
        }
    }

private:
    Promise<std::variant<Result<Ts>...>> promise_;

    lines::Atomic<size_t> last_;

    lines::Atomic<bool> done_ = false;
};

template <typename... Ts>
class DataForAnyWithoutException {
public:
    DataForAnyWithoutException(Promise<std::variant<Ts...>>&& promise, size_t future_cnt)
        : promise_(std::move(promise)), last_(future_cnt) {
    }

    ~DataForAnyWithoutException() = default;

    template <size_t Index>
    void SetResultOrIgnoreException(
        Result<typename std::tuple_element<Index, std::tuple<Ts...>>::type>&& result) {
        bool expected = false;

        if (result.has_value()) {
            if (done_.compare_exchange_strong(expected, true)) {
                std::move(promise_).SetValue(
                    std::variant<Ts...>{std::in_place_index<Index>, std::move(result).value()});
            }
        }

        if (--last_ == 0) {
            if (!done_.load() && !result.has_value()) {
                std::move(promise_).SetError(result.error());
            }

            delete this;
        }
    }

private:
    Promise<std::variant<Ts...>> promise_;

    lines::Atomic<size_t> last_;

    lines::Atomic<bool> done_ = false;
};

}  // namespace detail

template <typename... Fs>
Future<std::tuple<Result<typename std::remove_cvref_t<Fs>::ValueType>...>> CollectAll(Fs... fs) {
    using ResultType = std::tuple<Result<typename std::remove_cvref_t<Fs>::ValueType>...>;

    auto [future, promise] = GetTied<ResultType>();

    detail::DataForAll<typename std::remove_cvref_t<Fs>::ValueType...>* data =
        new detail::DataForAll(std::move(promise));

    detail::ForEach(
        [data](auto index, auto&& future) {
            constexpr size_t kIdx = decltype(index)::value;

            std::move(future).Subscribe([data](auto&& result) mutable noexcept {
                data->template AddResult<kIdx>(std::move(result));
            });
        },
        std::move(fs)...);

    return std::move(future);
}

template <typename... Fs>
Future<std::tuple<typename std::remove_cvref_t<Fs>::ValueType...>> Collect(Fs... fs) {
    using ResultType = std::tuple<typename std::remove_cvref_t<Fs>::ValueType...>;

    auto [future, promise] = GetTied<ResultType>();

    detail::Data<typename std::remove_cvref_t<Fs>::ValueType...>* data =
        new detail::Data(std::move(promise));

    detail::ForEach(
        [data](auto index, auto&& future) {
            constexpr size_t kIdx = decltype(index)::value;

            std::move(future).Subscribe([data](auto&& result) mutable noexcept {
                data->template AddValueOrSetError<kIdx>(std::move(result));
            });
        },
        std::move(fs)...);

    return std::move(future);
}

template <typename... Fs>
Future<std::variant<Result<typename std::remove_cvref_t<Fs>::ValueType>...>> CollectAny(Fs... fs) {
    using ResultType = std::variant<Result<typename std::remove_cvref_t<Fs>::ValueType>...>;

    auto [future, promise] = GetTied<ResultType>();

    detail::DataForAny<typename std::remove_cvref_t<Fs>::ValueType...>* data =
        new detail::DataForAny(std::move(promise), sizeof...(Fs));

    detail::ForEach(
        [data](auto index, auto&& future) {
            constexpr size_t kIdx = decltype(index)::value;

            std::move(future).Subscribe([data](auto&& result) mutable noexcept {
                data->template SetResult<kIdx>(std::move(result));
            });
        },
        std::move(fs)...);

    return std::move(future);
}

template <typename... Fs>
Future<std::variant<typename std::remove_cvref_t<Fs>::ValueType...>> CollectAnyWithoutException(
    Fs... fs) {
    using ResultType = std::variant<typename std::remove_cvref_t<Fs>::ValueType...>;

    auto [future, promise] = GetTied<ResultType>();

    detail::DataForAnyWithoutException<typename std::remove_cvref_t<Fs>::ValueType...>* data =
        new detail::DataForAnyWithoutException(std::move(promise), sizeof...(Fs));

    detail::ForEach(
        [data](auto index, auto&& future) {
            constexpr size_t kIdx = decltype(index)::value;

            std::move(future).Subscribe([data](auto&& result) mutable noexcept {
                data->template SetResultOrIgnoreException<kIdx>(std::move(result));
            });
        },
        std::move(fs)...);

    return std::move(future);
}

}  // namespace future
