#ifndef CPP_BASE_DATA_STRUCT_PCQUEUE_ABSTRACT_NONBLOCKING_PC_QUEUE_H_
#define CPP_BASE_DATA_STRUCT_PCQUEUE_ABSTRACT_NONBLOCKING_PC_QUEUE_H_

#include "cpp-base/macros.h"

namespace cpp_base {

// The parent of the different non-blocking queue implementations. This
// interface does not enforce single/multi producer/consumer functionality
// and the different implementations of this non-blocking queue interface
// can be of either nature.
template <typename T>
class AbstractNonblockingPcQueue {
  public:
    AbstractNonblockingPcQueue() {}
    virtual ~AbstractNonblockingPcQueue() {}

    virtual bool TryPut(T&& element) = 0;
    virtual bool TryGet(T* element) = 0;
    virtual int Capacity() const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(AbstractNonblockingPcQueue);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_PCQUEUE_ABSTRACT_NONBLOCKING_PC_QUEUE_H_
