#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <memory>

#define STACK_SIZE 1024 * 8

struct ITrampoline {
  virtual void Run() = 0;
};

void StaticTrampoline(void*, void*, void*, void*, void*, void*, void* task) {
  reinterpret_cast<ITrampoline*>(task)->Run();
}

extern "C" void SwitchContext(void* from_rsp, void* to_rsp);

extern "C" void* SetupContext(void* rsp, void* task, void* trampline_address);

class Context {
 public:
  Context() = default;

  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;

  void Setup(ITrampoline *task) {
    if (is_mapped) {
      munmap(stack, STACK_SIZE);
    }

    stack = mmap(NULL, STACK_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, 0, 0);

    void* stack_top = reinterpret_cast<char*>(stack) + STACK_SIZE;

    rsp = SetupContext(stack_top, task, reinterpret_cast<void*>(&StaticTrampoline));

    is_mapped = true;
  }

  void SwitchTo(Context *ctx) { 
    SwitchContext(&rsp, &ctx->rsp); 
  }

  ~Context() {
    if (is_mapped) {
      munmap(stack, STACK_SIZE);
    }
  }

 private:
  void* rsp;
  void* stack;
  bool is_mapped = false;
};
