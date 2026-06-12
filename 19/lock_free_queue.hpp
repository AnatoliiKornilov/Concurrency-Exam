#include <atomic>
#include <memory>

template <typename T>
class MSQueue {
  struct Node {
    T data;
    std::atomic<Node*> next;

    Node(const T& val) : data(val), next(nullptr) {}
  };

  std::atomic<Node*> head_;
  std::atomic<Node*> tail_;

 public:
  MSQueue() {
    Node* dummy = new Node(T{});

    head_.store(dummy);
    tail_.store(dummy);
  }

  ~MSQueue() {
    while ((Node* current = head_.load()) != nullptr) {
      head_.store(current->next.load());
      delete current;
    }
  }

  void push(const T& value) {
    Node* new_tail = new Node(value);

    while (true) {
      Node* tail = tail.load();
      Node* next = tail->next.load();

      if (tail != tail_.load()) {
        continue;
      }

      if (next != nullptr) {
        tail_.compare_exchange_strong(tail, next);
        continue;
      }

      Node* expected = nullptr;
      if (tail->next.compare_exchange_strong(expected, new_tail)) {
        tail_.compare_exchange_strong(tail, new_tail);
        return;
      }
    }
  }

  std::optional<T> pop() {
    while (true) {
      Node* head = head_.load();
      Node* tail = tail_.load();
      Node* next_head = head->next.load();

      if (head != head_.load()) {
        continue;
      }

      if (head == tail) {
        if (next_head == nullptr) {
          return std::nullopt;
        }

        tail_.compare_exchange_strong(tail, next_head);
        continue;
      }

      T result = next_head->data;
      if (head_.compare_exchange_strong(head, next_head)) {
        delete head; // опасно
        return result;
      }
    }
  }
};
