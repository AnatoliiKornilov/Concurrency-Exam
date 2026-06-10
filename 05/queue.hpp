#pragma once

#include <lines/std/condvar.hpp>
#include <lines/std/mutex.hpp>
#include <optional>
#include <queue>

template <class T>
class MPMCBlockingUnboundedQueue {
 public:
  void Push(T elem) {
    mutex_.Lock();

    if (closed_) {
      mutex_.Unlock();
      return;
    }

    queue_.push(elem);

    cv_.NotifyOne();

    mutex_.Unlock();
  }

  std::optional<T> Pop() {
    mutex_.Lock();

    while (!closed_ && queue_.empty()) {
      cv_.Wait(mutex_);
    }

    if (closed_ && queue_.empty()) {
      mutex_.Unlock();
      return std::nullopt;
    }

    T elem = std::move(queue_.front());

    queue_.pop();

    mutex_.Unlock();

    return elem;
  }

  void Close() {
    mutex_.Lock();

    closed_ = true;

    cv_.NotifyAll();

    mutex_.Unlock();
  }

 private:
  bool closed_ = false;

  std::queue<T> queue_;

  lines::Mutex mutex_;
  lines::Condvar cv_;
};
