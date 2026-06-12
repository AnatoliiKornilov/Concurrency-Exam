#include <atomic>
#include <memory>
#include <optional>

template <typename T>
class TreiberStack {
 public:
  void push(const T& value) {
    Node* new_node = new Node(value);

    new_node->next = head_.load();

    while (!head_.compare_exchange_strong(new_node->next, new_node)) {}
  }

  std::optional<T> pop() {
    Node* old_head = head_.load();

    while (old_head && !head_.compare_exchange_strong(old_head, old_head->next)) {}

    if (!old_head) {
      return std::nullopt;
    }

    T result = old_head->value;

    delete old_head; // опасно

    return result;
  }

  ~TreiberStack() {
    while (true) {
      Node* top = head.load();
      if (top == nullptr) break;
      if (head.compare_exchange_strong(top, top->next)) {
        delete top;
      }
    }
  }

 private:
  struct Node {
    T data;
    Node* next;
    Node(const T& d, Node* n = nullptr) : data(d), next(n) {}
  };

  std::atomic<Node*> head_ = nullptr;
};
