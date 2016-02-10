#ifndef CPP_BASE_DATA_STRUCT_PCQUEUE_ABSTRACT_BLOCKING_PC_QUEUE_H_
#define CPP_BASE_DATA_STRUCT_PCQUEUE_ABSTRACT_BLOCKING_PC_QUEUE_H_

#include <memory>
#include "cpp-base/data-struct/pcqueue/abstract_nonblocking_pc_queue.h"
#include "cpp-base/macros.h"

namespace cpp_base {

// The parent of the different blocking queue implementations. Currently, a
// blocking queue is simply a wrapper over a non-blocking queue and it provides
// 'blocking' functionality via either a spin lock or a semaphore.
// NOTE: in here, being single/multiple-producer single/multiple-consumer queue
// has nothing to do with and is an orthogonal matter to the matter of being
// blocking or non-blocking: this class provides a blocking queue either
// single/multi producer/consumer, depending on the underlying non-blocking
// queue given to it.
template <typename T>
class AbstractBlockingPcQueue {
  public:
    // Takes ownership of the 'queue' pointer.
    explicit AbstractBlockingPcQueue(AbstractNonblockingPcQueue<T>* queue)
            : queue_(queue) {}
    virtual ~AbstractBlockingPcQueue() {}

    virtual void Put(T&& element) = 0;
    virtual bool TryPut(T&& element) = 0;

    virtual void Get(T* element) = 0;
    virtual bool TryGet(T* element) = 0;

  protected:
    std::unique_ptr<AbstractNonblockingPcQueue<T>> queue_;

  private:
    DISALLOW_COPY_AND_ASSIGN(AbstractBlockingPcQueue);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_PCQUEUE_ABSTRACT_BLOCKING_PC_QUEUE_H_
