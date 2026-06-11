#include <linux/futex.h>
#include <sys/syscall.h>

#include <atomic>
#include <cstdint>
#include <mutex>

class CondVar {
  std::atomic<std::uint32_t> gen_num = 0;

 public:

  void wait(std::mutex& mutex) {
    std::uint32_t generation = gen_num.load();

    mutex.unlock();

    syscall(SYS_futex, &gen_num, FUTEX_WAIT, generation, nullptr, nullptr, 0);

    mutex.lock();
  }

  void notify_one() {
    ++gen_num;

    syscall(SYS_futex, &gen_num, FUTEX_WAKE, 1, nullptr, nullptr, 0);
  }

  void notify_all() {
    ++gen_num;

    syscall(SYS_futex, &gen_num, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
  }
};
