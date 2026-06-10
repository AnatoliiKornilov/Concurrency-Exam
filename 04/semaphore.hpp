// g++ semaphore.cpp -std=c++23 -o semaphore

#include <condition_variable>
#include <mutex>

class Semaphore {
 public:

  explicit Semaphore(std::size_t initial = 1) : count_(initial) {}


  void acquire() {
    std::unique_lock<std::mutex> lock(mutex_);

    while (count_ == 0) {
      cv_.wait(lock);
    }

    --count_;
  }

  void release() {
    std::lock_guard<std::mutex> lock(mutex_);

    ++count_;

    cv_.notify_one();
  }

 private:

  std::size_t count_ = 0;

  std::mutex mutex_;
  std::condition_variable cv_;
};
