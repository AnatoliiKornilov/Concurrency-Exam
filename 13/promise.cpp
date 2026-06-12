#include "promise.hpp"

namespace future {

const char* PromiseException::what() const noexcept {
    return "Promise error";
}

const char* PromiseInvalid::what() const noexcept {
    return "Invalid promise";
}

const char* PromiseAlreadySatisfied::what() const noexcept {
    return "Promise already satisfied";
}

const char* PromiseBroken::what() const noexcept {
    return "Promise is broken";
}

}  // namespace future
