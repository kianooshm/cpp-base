#ifndef CPP_BASE_DATA_STRUCT_PCQUEUE_MPMC_QUEUE_BY_MUTEX_H_
#define CPP_BASE_DATA_STRUCT_PCQUEUE_MPMC_QUEUE_BY_MUTEX_H_

#include "cpp-base/data-struct/pcqueue/abstract_nonblocking_pc_queue.h"
#include "cpp-base/data-struct/ring_buffer.h"
#include "cpp-base/mutex.h"

namespace cpp_base {

// Multiple-producer multiple-consumer queue using a mutex lock over the whole queue.
template<typename T>
class MpmcQueueByMutex : public AbstractNonblockingPcQueue<T> {
  public:
    explicit MpmcQueueByMutex(int capacity) : buffer_(capacity) {}
    virtual ~MpmcQueueByMutex() {}

    virtual bool TryPut(T&& element) override {
        cpp_base::MutexLock lock(&mutex_);
        return buffer_.TryPut(std::forward<T>(element));
    }

    virtual bool TryGet(T* element) override {
        cpp_base::MutexLock lock(&mutex_);
        return buffer_.TryGet(element);
    }

    virtual int Capacity() const { return buffer_.Capacity(); }

  private:
    cpp_base::Mutex mutex_;
    cpp_base::RingBuffer<T> buffer_;
    DISALLOW_COPY_AND_ASSIGN(MpmcQueueByMutex);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_PCQUEUE_MPMC_QUEUE_BY_MUTEX_H_
