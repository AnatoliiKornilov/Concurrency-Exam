#pragma once

#include <lines/std/atomic.hpp>
#include <lines/std/thread.hpp>
#include <vector>

#include "queue.hpp"
#include "task.hpp"
#include "wg.hpp"

class ThreadPool {
 public:
  explicit ThreadPool(std::size_t num_threads);
  ~ThreadPool();

  void Submit(Task task);

  void Wait();

  static ThreadPool* This();

 private:
  void Work();

 private:
  static thread_local ThreadPool* current_pool_;

  std::vector<lines::Thread> threads_;
  MPMCBlockingUnboundedQueue<Task> tasks_;
  WaitGroup wg_;

  lines::Mutex mutex_;
};
