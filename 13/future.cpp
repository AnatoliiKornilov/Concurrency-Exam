#include "future.hpp"

namespace future {

const char* FutureException::what() const noexcept {
    return "Future error";
}

const char* FutureInvalid::what() const noexcept {
    return "Future is invalid";
}

const char* FutureNotReady::what() const noexcept {
    return "Future is not ready";
}

}  // namespace future
