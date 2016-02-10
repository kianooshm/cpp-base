#include <gtest/gtest.h>
#include <algorithm>
#include <deque>
#include "cpp-base/data-struct/ring_buffer.h"

template <typename T>
class DequeBasedRingBuffer {
  public:
    explicit DequeBasedRingBuffer(int capacity) : capacity_(capacity) {}
    ~DequeBasedRingBuffer() {}

    bool TryPut(T&& element) {
        if (queue_.size() >= capacity_)
            return false;
        queue_.push_back(std::forward<T>(element));
        return true;
    }
    bool TryGet(T* element) {
        if (queue_.empty())
            return false;
        *element = queue_.front();
        queue_.pop_front();
        return true;
    }
    int Count()  const { return queue_.size(); }
    bool Empty() const { return queue_.empty(); }
    bool Full() const { return queue_.size() >= capacity_; }

  private:
    const int capacity_;
    std::deque<T> queue_;
};

class RingBufferTest : public ::testing::Test {
  public:
    RingBufferTest() {}
    ~RingBufferTest() {}

  protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RingBufferTest, ComprehensiveTest) {
    cpp_base::RingBuffer<int> buff1(100);
    DequeBasedRingBuffer<int> buff2(100);
    int max_size = 0;
    srand(1);
    for (int i = 0; i < 1000000; ++i) {
        bool put = rand() % 2;
        if (put) {
            int num = rand();
            EXPECT_EQ(buff1.TryPut(num + 0), buff2.TryPut(num + 0));
        } else {  // get
            int num1, num2;
            bool ret = buff1.TryGet(&num1);
            EXPECT_EQ(buff2.TryGet(&num2), ret);
            if (ret) {
                EXPECT_EQ(num1, num2);
            }
        }
        EXPECT_EQ(buff1.Count(), buff2.Count());
        EXPECT_EQ(buff1.Empty(), buff2.Empty());
        EXPECT_EQ(buff1.Full(), buff2.Full());
        max_size = std::max(max_size, buff1.Count());
    }
    LOG(INFO) << "Max size = " << max_size;
}
