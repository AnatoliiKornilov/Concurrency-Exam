#pragma once

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>

class Mutex {
  std::atomic<int> locked_{0};

 public:
  void lock() {
    int expected = 0;

    if (locked_.exchange(1) == 0) {
      return;
    }

    do {
      syscall(SYS_futex, &locked_, FUTEX_WAIT, 1, nullptr, nullptr, 0);
    } while (locked_.exchange(1, std::memory_order_acquire) == 1);
  }

  void unlock() {
    locked_.store(0);

    syscall(SYS_futex, &locked_, FUTEX_WAKE, 1, nullptr, nullptr, 0);
  }
};
