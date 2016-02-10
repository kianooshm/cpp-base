#ifndef CPP_BASE_DATA_STRUCT_PCQUEUE_SPSC_QUEUE_BY_FOLLY_H_
#define CPP_BASE_DATA_STRUCT_PCQUEUE_SPSC_QUEUE_BY_FOLLY_H_

#include <glog/logging.h>
#include <atomic>
#include "cpp-base/data-struct/pcqueue/abstract_nonblocking_pc_queue.h"
#include "cpp-base/macros.h"
#include "cpp-base/type_traits.h"

namespace cpp_base {

// Single-producer Single-consumer queue by Facebook's Folly library.
// Source: https://github.com/facebook/folly/blob/master/folly/ProducerConsumerQueue.h
// Doc:    https://github.com/facebook/folly/blob/master/folly/docs/ProducerConsumerQueue.md
template <typename T>
class SpscQueueByFolly : public AbstractNonblockingPcQueue<T> {
  public:
    // capacity must be >= 2.
    // Also, note that the number of usable slots in the queue at any
    // given time is actually (size-1), so if you start with an empty queue,
    // isFull() will return true after size-1 insertions.
    explicit SpscQueueByFolly(int size)
            : capacity_(size),
              records_(static_cast<T*>(std::malloc(sizeof(T) * size))),
              readIndex_(0),
              writeIndex_(0) {
        CHECK_GE(size, 2);
        CHECK(records_ != NULL);
    }

    ~SpscQueueByFolly() {
        // We need to destruct anything that may still exist in our queue.
        // (No real synchronization needed at destructor time: only one
        // thread can be doing this.)
        if (!cpp_base::has_trivial_destructor<T>::value) {
            int read = readIndex_;
            int end = writeIndex_;
            while (read != end) {
                records_[read].~T();
                if (++read == capacity_)
                    read = 0;
            }
        }
        std::free(records_);
    }

#if 1
    bool TryPut(T&& element) override {      // NOLINT
        auto const currentWrite = writeIndex_.load(std::memory_order_relaxed);
        auto nextRecord = currentWrite + 1;
        if (nextRecord == capacity_) {
            nextRecord = 0;
        }
        if (nextRecord != readIndex_.load(std::memory_order_acquire)) {
            new (&records_[currentWrite]) T(std::forward<T>(element));
            writeIndex_.store(nextRecord, std::memory_order_release);
            return true;
        }
        // queue is full
        return false;
    }
#else
    template<class ...Args>
    bool TryPut(Args&&... recordArgs) override {     // NOLINT
        auto const currentWrite = writeIndex_.load(std::memory_order_relaxed);
        auto nextRecord = currentWrite + 1;
        if (nextRecord == capacity_) {
            nextRecord = 0;
        }
        if (nextRecord != readIndex_.load(std::memory_order_acquire)) {
            new (&records_[currentWrite]) T(std::forward<Args>(recordArgs)...);
            writeIndex_.store(nextRecord, std::memory_order_release);
            return true;
        }
        // queue is full
        return false;
    }
#endif

    // Move (or copy) the value at the front of the queue to given variable.
    bool TryGet(T* record) override {
        auto const currentRead = readIndex_.load(std::memory_order_relaxed);
        if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
          // queue is empty
          return false;
        }

        auto nextRecord = currentRead + 1;
        if (nextRecord == capacity_) {
          nextRecord = 0;
        }
        *record = std::forward<T>(records_[currentRead]);
        records_[currentRead].~T();
        readIndex_.store(nextRecord, std::memory_order_release);
        return true;
    }

    // Pointer to the value at the front of the queue (for use in-place) or
    // nullptr if empty.
    T* FrontPtr() {
        auto const currentRead = readIndex_.load(std::memory_order_relaxed);
        if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
          // queue is empty
          return nullptr;
        }
        return &records_[currentRead];
    }

    // Queue must not be empty.
    void PopFront() {
        auto const currentRead = readIndex_.load(std::memory_order_relaxed);
        CHECK_NE(currentRead, writeIndex_.load(std::memory_order_acquire));

        auto nextRecord = currentRead + 1;
        if (nextRecord == capacity_) {
          nextRecord = 0;
        }
        records_[currentRead].~T();
        readIndex_.store(nextRecord, std::memory_order_release);
    }

    bool Empty() const {
        return readIndex_.load(std::memory_order_consume) ==
               writeIndex_.load(std::memory_order_consume);
    }

    bool Full() const {
        auto nextRecord = writeIndex_.load(std::memory_order_consume) + 1;
        if (nextRecord == capacity_) {
            nextRecord = 0;
        }
        if (nextRecord != readIndex_.load(std::memory_order_consume)) {
            return false;
        }
        // queue is full
        return true;
    }

    // * If called by consumer, then true size may be more (because producer may
    //   be adding items concurrently).
    // * If called by producer, then true size may be less (because consumer may
    //   be removing items concurrently).
    // * It is undefined to call this from any other thread.
    int SizeGuess() const {
        int ret = writeIndex_.load(std::memory_order_consume) -
                  readIndex_.load(std::memory_order_consume);
        if (ret < 0) {
            ret += capacity_;
        }
        return ret;
    }

    int Capacity() const override { return capacity_; }

  private:
    const int capacity_;
    T* const records_;
    std::atomic<int> readIndex_;
    std::atomic<int> writeIndex_;

    DISALLOW_COPY_AND_ASSIGN(SpscQueueByFolly);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_PCQUEUE_SPSC_QUEUE_BY_FOLLY_H_
