#include "tp.hpp"

#include <lines/util/thread_local.hpp>

thread_local ThreadPool* ThreadPool::current_pool_ = nullptr;

ThreadPool::ThreadPool(std::size_t num_threads) {
  threads_.reserve(num_threads);

  for (std::size_t i = 0; i < num_threads; ++i) {
    threads_.emplace_back([this]() { Work(); });
  }
}

ThreadPool::~ThreadPool() {
  tasks_.Close();

  for (lines::Thread& thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void ThreadPool::Submit(Task task) {
  wg_.Add(1);

  tasks_.Push([this, task = std::move(task)]() {
    task();

    wg_.Done();
  });
}

void ThreadPool::Wait() { wg_.Wait(); }

ThreadPool* ThreadPool::This() { return current_pool_; }

void ThreadPool::Work() {
  current_pool_ = this;

  while (true) {
    std::optional<Task> task = tasks_.Pop();

    if (!task.has_value()) {
      break;
    }

    (*task)();
  }

  current_pool_ = nullptr;
}
