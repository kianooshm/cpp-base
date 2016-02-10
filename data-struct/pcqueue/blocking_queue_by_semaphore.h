#ifndef CPP_BASE_DATA_STRUCT_PCQUEUE_BLOCKING_QUEUE_BY_SEMAPHORE_H_
#define CPP_BASE_DATA_STRUCT_PCQUEUE_BLOCKING_QUEUE_BY_SEMAPHORE_H_

#include <glog/logging.h>
#include <semaphore.h>
#include "cpp-base/data-struct/pcqueue/abstract_blocking_pc_queue.h"
#include "cpp-base/macros.h"

namespace cpp_base {

// A blocking by semaphore will, upon a possibly blocking queue operation, sleep
// on a semaphore to be woken up when the queue operation becomes possible.
// See also BlockingQueueBySpinLock.
template <typename T>
class BlockingQueueBySemaphore : public AbstractBlockingPcQueue<T> {
  public:
    // We initialize the 'full' semaphore with 0 and the 'empty' semaphore with
    // QueueCapacity-1 (rather than QueueCapacity itself), since experiments
    // show that some of our non-blocking queue implementations can be off by 1
    // in e.g. returning false on TryPut() or TryGet().
    // Takes ownership of the 'queue' pointer.
    explicit BlockingQueueBySemaphore(AbstractNonblockingPcQueue<T>* queue)
            : AbstractBlockingPcQueue<T>(queue),
              full_(0),
              empty_(queue->Capacity() - 1) {  // Assume the capacity is 1 less
        CHECK_GE(queue->Capacity(), 2);
    }

    void Put(T&& element) override {
        empty_.Lock();
        CHECK(this->queue_->TryPut(std::forward<T>(element)));
        full_.Unlock();
    }

    bool TryPut(T&& element) override {
        if (!empty_.TryLock())
            return false;
        CHECK(this->queue_->TryPut(std::forward<T>(element)));
        full_.Unlock();
        return true;
    }

    void Get(T* element) override {
        full_.Lock();
        CHECK(this->queue_->TryGet(element));
        empty_.Unlock();
    }


    bool TryGet(T* element) override {
        if (!full_.TryLock())
            return false;
        CHECK(this->queue_->TryGet(element));
        empty_.Unlock();
        return true;
    }

    bool Empty() { return (full_.GetValue() == 0); }

  private:
    class Semaphore {
      public:
        Semaphore(unsigned int value) { sem_init(&sem_, 0, value); }
        ~Semaphore() { sem_destroy(&sem_); }
        inline void Lock() { sem_wait(&sem_); }
        inline void Unlock() { sem_post(&sem_); }
        inline bool TryLock() { return sem_trywait(&sem_) == 0; }
        inline int GetValue() {
            int ret = -1;
            sem_getvalue(&sem_, &ret);
            return ret;
        }
      private:
        sem_t sem_;
    };
    Semaphore full_;
    Semaphore empty_;

    DISALLOW_COPY_AND_ASSIGN(BlockingQueueBySemaphore);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_PCQUEUE_BLOCKING_QUEUE_BY_SEMAPHORE_H_
