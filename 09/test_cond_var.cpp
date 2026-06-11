// g++ test_cond_var.cpp -std=c++23 -pthread -o test_cond_var

#include "cond_var.hpp"

#include <chrono>
#include <iostream>
#include <thread>

std::mutex mtx;
CondVar cv;
bool done = false;

void worker() {
  std::unique_lock lock(mtx);
  while (!done) {
    cv.wait(mtx);
  }
  std::cout << "Worker finished\n";
}

int main() {
  std::thread t(worker);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  {
    std::lock_guard lock(mtx);
    done = true;
  }
  cv.notify_one();
  t.join();
  std::cout << "Test passed\n";
}
