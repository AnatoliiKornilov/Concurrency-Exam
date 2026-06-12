#include <unistd.h>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
 public:
  ThreadPool(std::size_t num) {
    for (std::size_t i = 0; i < num; ++i) {
      workers_.emplace_back([this]() { 
        worker(); 
      });
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard lock(mutex_);
      stop_ = true;
    }

    cv_.notify_all();

    for (std::thread& thread : workers_) {
      thread.join();
    }
  }

  void enqueue(std::function<void()> task) {
    {
      std::lock_guard lock(mutex_);
      tasks_.push(std::move(task));
    }

    cv_.notify_one();
  }

 private:
  void worker() {
    while (true) {
      std::function<void()> task;

      {
        std::unique_lock lock(mutex_);

        cv_.wait(lock, [this]() { return !tasks_.empty() || stop_; });

        if (stop_ && tasks_.empty()) {
          return;
        }

        task = std::move(tasks_.front());
        tasks_.pop();
      }

      task();
    }
  }

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;

  std::mutex mutex_;
  std::condition_variable cv_;

  bool stop_ = false;
};
