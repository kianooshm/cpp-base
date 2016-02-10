#ifndef CPP_BASE_DATA_STRUCT_PCQUEUE_SPSC_QUEUE_BY_DRDOBBS_H_
#define CPP_BASE_DATA_STRUCT_PCQUEUE_SPSC_QUEUE_BY_DRDOBBS_H_

#include <atomic>
#include "cpp-base/data-struct/pcqueue/abstract_nonblocking_pc_queue.h"

namespace cpp_base {

// Single-producer Single-consumer queue.
// Source: Dr Dobb's -- Writing Lock-Free Code: A Corrected Queue
// http://www.drdobbs.com/parallel/writing-lock-free-code-a-corrected-queue/210604448?pgno=1
template <typename T>
class SpscQueueByDrDobbs : public AbstractNonblockingPcQueue<T> {
 public:
    explicit SpscQueueByDrDobbs(int capacity) : capacity_(capacity), approx_size_(0) {
    first = divider = last = new Node(T());  // add dummy separator
  }

  virtual ~SpscQueueByDrDobbs() {
    while (first != nullptr) {   // release the list
      Node* tmp = first;
      first = tmp->next;
      delete tmp;
    }
  }

  bool TryPut(T&& element) override {
    if (approx_size_ >= capacity_)
        return false;
    (*last).next = new Node(std::forward<T>(element));  // add the new item
    last = (*last).next;               // publish it
    while (first != divider) {         // trim unused nodes
      Node* tmp = first;
      first = (*first).next;
      delete tmp;
    }
    ++approx_size_;
    return true;
  }

  bool TryGet(T* element) override {
    if (divider != last) {                // if queue is nonempty
      *element = std::forward<T>((*divider).next->value);  // C: copy it back
      divider = (*divider).next;          // D: publish that we took it
      --approx_size_;
      return true;                        // and report success
    }
    return false;                         // else report empty
  }

  int Capacity() const override { return capacity_; }

 private:
  struct Node {
    Node(T val) : value(val), next(nullptr) { }
    T value;
    Node* next;
  };

  const int capacity_;
  std::atomic<int> approx_size_;
  Node* first;                       // for producer only
  std::atomic<Node*> divider, last;  // shared
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_PCQUEUE_SPSC_QUEUE_BY_DRDOBBS_H_
