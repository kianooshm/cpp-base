#ifndef CPP_BASE_DATA_STRUCT_PCQUEUE_BLOCKING_QUEUE_BY_SPINLOCK_H_
#define CPP_BASE_DATA_STRUCT_PCQUEUE_BLOCKING_QUEUE_BY_SPINLOCK_H_

#include <glog/logging.h>
#include "cpp-base/data-struct/pcqueue/abstract_blocking_pc_queue.h"
#include "cpp-base/macros.h"

namespace cpp_base {

// A blocking queue by spin lock will, upon a possibly blocking queue operation,
// continually monitor the queue in a busy wait until the operation can be done.
// A sched_yield() in the busy wait allows to give up the CPU to other threads
// WITHOUT a sleep. Sleep and wake-up are rather expensive OS calls, either via
// semaphores or e.g. a microsecond-sleep in the busy wait.
// The choice between the CPU intensity of a busy wait (i.e. a spin-lock-based
// blocking queue) and the overhead of sleep and wake-up (i.e. a semaphore-based
// blocking queue) depends on your application: how many threads are there and
// how short a wait on the queue is expected to be.
template <typename T>
class BlockingQueueBySpinLock : public AbstractBlockingPcQueue<T> {
  public:
    // Takes ownership of the 'queue' pointer.
    explicit BlockingQueueBySpinLock(AbstractNonblockingPcQueue<T>* queue)
            : AbstractBlockingPcQueue<T>(queue) {}

    bool TryPut(T&& element) override {
        return this->queue_->TryPut(std::forward<T>(element));
    }

    bool TryGet(T* element) override {
        return this->queue_->TryGet(element);
    }

    void Put(T&& element) override { PutWithRetrySleep(std::forward<T>(element), 0); }

    void Get(T* element) override { GetWithRetrySleep(element, 0); }

    // Keeps trying to put in the queue until success. Yields the CPU between retries. If a value
    // of >0 is given for sleep_between_retries_usec, sleeps for that many microseconds between
    // retries.
    inline void PutWithRetrySleep(T&& element, int sleep_between_retries_usec) {
        while (!this->queue_->TryPut(std::forward<T>(element)))
            if (sleep_between_retries_usec <= 0)
                sched_yield();
            else
                usleep(sleep_between_retries_usec);
    }

    // Keeps trying to get from the queue until success. Yields the CPU between retries. If a value
    // of >0 is given for sleep_between_retries_usec, sleeps for that many microseconds between
    // retries.
    inline void GetWithRetrySleep(T* element, int sleep_between_retries_usec) {
        while (!this->queue_->TryGet(element))
            if (sleep_between_retries_usec <= 0)
                sched_yield();
            else
                usleep(sleep_between_retries_usec);
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(BlockingQueueBySpinLock);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_PCQUEUE_BLOCKING_QUEUE_BY_SPINLOCK_H_
