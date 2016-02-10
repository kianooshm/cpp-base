#ifndef CPP_BASE_DATA_STRUCT_RING_BUFFER_H_
#define CPP_BASE_DATA_STRUCT_RING_BUFFER_H_

#include <glog/logging.h>
#include <vector>
#include "cpp-base/macros.h"

namespace cpp_base {

// Fixed-size, circular FIFO buffer on a vector. This class is not thread safe.
template <typename T>
class RingBuffer {
  public:
    explicit RingBuffer(int capacity) : capacity_(capacity), data_(capacity) {
        CHECK_GT(capacity_, 0);
    }
    ~RingBuffer() {}

    // Returns whether the element is successfully put, i.e., buffer not full.
    bool TryPut(T&& element) {
        CHECK_LE(count_, capacity_);
        if (count_ == capacity_) {
            return false;
        }
        int end = (front_ + count_) % capacity_;
        data_[end] = std::forward<T>(element);
        count_++;
        return true;
    }

    // Returns whether successfully got an element, i.e., buffer not empty.
    bool TryGet(T* element) {
        CHECK_GE(count_, 0);
        if (count_ == 0) {
            return false;
        }
        *element = std::forward<T>(data_[front_]);
        front_ = (front_ + 1) % capacity_;
        count_--;
        return true;
    }

    int Count()    const { return count_; }
    int Capacity() const { return capacity_; }
    bool Empty()   const { return count_ == 0; }
    bool Full()    const { return count_ == capacity_; }

  private:
    const int capacity_;
    int front_ = 0;
    int count_ = 0;
    std::vector<T> data_;

    DISALLOW_COPY_AND_ASSIGN(RingBuffer);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_RING_BUFFER_H_
