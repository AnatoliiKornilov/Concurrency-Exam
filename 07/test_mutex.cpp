// g++ test_mutex.cpp -std=c++23 -pthread -o test_mutex

#include "mutex.hpp"

#include <cassert>
#include <thread>
#include <vector>

Mutex mtx;
int counter = 0;

void worker(int iterations) {
  for (int i = 0; i < iterations; ++i) {
    mtx.lock();
    ++counter;
    mtx.unlock();
  }
}

int main() {
  const int THREADS = 40;
  const int ITER = 100;
  std::vector<std::thread> threads;

  for (int i = 0; i < THREADS; ++i) {
    threads.emplace_back(worker, ITER);
  }

  for (std::thread& thread : threads) {
    thread.join();
  }

  assert(THREADS * ITER == counter);

  return 0;
}
