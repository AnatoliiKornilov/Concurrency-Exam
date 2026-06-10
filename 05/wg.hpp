#pragma once

#include <lines/std/condvar.hpp>
#include <lines/std/mutex.hpp>
#include <mutex>

class WaitGroup {
 public:
  void Add(std::size_t count) {
    mutex_.Lock();

    counter_ += count;

    mutex_.Unlock();
  }

  void Done() {
    mutex_.Lock();

    if (counter_ == 0) {
      mutex_.Unlock();
      return;
    }

    if (--counter_ == 0) {
      cv_.NotifyAll();
    }

    mutex_.Unlock();
  }

  void Wait() {
    mutex_.Lock();

    while (counter_ > 0) {
      cv_.Wait(mutex_);
    }

    mutex_.Unlock();
  }

 private:
  std::size_t counter_ = 0;

  lines::Mutex mutex_;
  lines::Condvar cv_;
};
