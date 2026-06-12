#include "context.cpp"

#include <functional>

struct Task : public ITrampoline {
  void Run() {
    printf("Hello\n");
    exit(0);
  }
};

class StackfullCoro;

void Yield();

thread_local StackfullCoro* prev_coro = nullptr;

class StackfullCoro : public ITrampoline {
 public:
  StackfullCoro(std::function<void()> task) : task(task) {
    task_ctx.Setup(this);
  }

  void Run() override {
    task();
    Yield();
  }

  void Resume() {
    save_prev_coro = prev_coro;
    prev_coro = this;
    main_ctx.SwitchTo(&task_ctx);
  }

 private:
  friend void Yield();

  std::function<void()> task;
  Context main_ctx;
  Context task_ctx;
  StackfullCoro* save_prev_coro;
};

void Yield() {
  if (prev_coro == nullptr) {
    exit(1);
  }
  StackfullCoro* target_coro = prev_coro;
  prev_coro = target_coro->save_prev_coro;
  target_coro->task_ctx.SwitchTo(&target_coro->main_ctx);
}

int main() {
  StackfullCoro coro([]() {
    printf("1\n");

    StackfullCoro coro([]() {
      printf("2\n");
      Yield();
      printf("6\n");
    });

    coro.Resume();
    printf("3\n");
    Yield();
    printf("5\n");
    coro.Resume();
  });

  coro.Resume();
  printf("4\n");
  coro.Resume();
  printf("7\n");
}
