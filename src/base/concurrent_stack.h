#ifndef CONCURRENT_STACK_H
#define CONCURRENT_STACK_H

#include <atomic>
#include <utility>

namespace base {

// Lock-free concurrent stack. All methods are thread-safe and can be called on
// any thread.
template <typename T>
class ConcurrentStack {
  struct Node {
    T item;
    Node* next = nullptr;
    Node(T item) : item(std::move(item)) {}
  };

 public:
  ConcurrentStack() = default;

  ConcurrentStack(ConcurrentStack<T>&& other) { MoveAssign(std::move(other)); }

  ~ConcurrentStack() { Clear(); }

  ConcurrentStack<T>& operator=(ConcurrentStack<T>&& other) {
    MoveAssign(std::move(other));
    return *this;
  }

  void Push(T item) {
    Node* new_node = new Node(std::move(item));
    new_node->next = head_.load(std::memory_order_relaxed);
    while (!head_.compare_exchange_weak(new_node->next, new_node,
                                        std::memory_order_release,
                                        std::memory_order_relaxed))
      ;
  }

  bool Pop(T& item) {
    Node* head_node = head_.load(std::memory_order_relaxed);
    if (!head_node)
      return false;
    while (!head_.compare_exchange_weak(head_node, head_node->next,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed)) {
      if (!head_node)
        return false;
    }

    item = std::move(head_node->item);
    delete head_node;
    return true;
  }

  void Clear() {
    Node* null_node = nullptr;
    Node* head_node = head_.load(std::memory_order_relaxed);
    while (!head_.compare_exchange_weak(head_node, null_node,
                                        std::memory_order_relaxed))
      ;

    while (head_node) {
      Node* next_node = head_node->next;
      delete head_node;
      head_node = next_node;
    }
  }

  bool Empty() const { return !head_.load(std::memory_order_relaxed); }

 private:
  std::atomic<Node*> head_ = nullptr;

  void MoveAssign(ConcurrentStack<T>&& other) {
    Node* null_node = nullptr;
    Node* head_node = other.head_.load(std::memory_order_relaxed);
    while (!other.head_.compare_exchange_weak(head_node, null_node,
                                              std::memory_order_relaxed))
      ;

    head_.store(head_node, std::memory_order_relaxed);
  }

  ConcurrentStack(const ConcurrentStack<T>&) = delete;
  ConcurrentStack<T>& operator=(const ConcurrentStack<T>&) = delete;
};

}  // namespace base

#endif  // CONCURRENT_STACK_H
