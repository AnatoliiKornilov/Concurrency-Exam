#pragma once

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>

class Mutex {
  std::atomic<int> state_{0};

 public:
  void lock() {
    while (true) {
      int expected = 0;

      if (state_.compare_exchange_strong(expected, 1)) {
        return;
      }

      expected = 1;

      state_.compare_exchange_strong(expected, 2);

      syscall(SYS_futex, &state_, FUTEX_WAIT, 2, nullptr, nullptr, 0);
    }
  }

  void unlock() {
    int old = state_.exchange(0);

    if (old == 2) {
      syscall(SYS_futex, &state_, FUTEX_WAKE, 1, nullptr, nullptr, 0);
    }
  }
};
