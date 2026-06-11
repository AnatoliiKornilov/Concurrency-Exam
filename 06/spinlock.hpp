#pragma once

#include <lines/fibers/api.hpp>
#include <lines/std/atomic.hpp>

class SpinLock {
 public:
  void Lock() {
    while (!is_free_.exchange(false)) {
      lines::Yield();
    }
  }

  void Unlock() { is_free_.store(true); }

 private:
  lines::Atomic<bool> is_free_{true};
};
