#include <gtest/gtest.h>
#include <algorithm>
#include <hash_map>
#include "cpp-base/data-struct/count_min_sketch.h"

using cpp_base::CountMinSketch;

class CountMinSketchTest : public ::testing::Test {
  public:
    CountMinSketchTest() { }
    ~CountMinSketchTest() { }

  protected:
    void SetUp() override { }
    void TearDown() override { }
};

TEST_F(CountMinSketchTest, BasicTest) {
    CountMinSketch<uint8> sketch(1000, 0.01);
    EXPECT_EQ(0, sketch.GetCount(100));

    // Increment the count of 100 by 3.
    EXPECT_EQ(3, sketch.AddCount(100, 3));
    EXPECT_EQ(3, sketch.GetCount(100));
    EXPECT_EQ(3, sketch.SumCounts());

    // Non-existent key 200.
    EXPECT_EQ(0, sketch.GetCount(200));

    // Decrement the count of 100 by 2.
    EXPECT_EQ(1, sketch.AddCount(100, -2));
    EXPECT_EQ(1, sketch.GetCount(100));
    EXPECT_EQ(1, sketch.SumCounts());

    // Try decrementing the count of 100 to below 0. Shouldn't go.
    EXPECT_EQ(0, sketch.AddCount(100, -10));
    EXPECT_EQ(0, sketch.GetCount(100));

    // Add the count of 200 by 5, then clear the counter and verify.
    EXPECT_EQ(5, sketch.AddCount(200, 5));
    EXPECT_EQ(5, sketch.GetCount(200));
    sketch.Clear();
    EXPECT_EQ(0, sketch.GetCount(200));
}

TEST_F(CountMinSketchTest, OverflowTest) {
    // Test overflow with uint8, uint16 and uint32:
    CountMinSketch<uint8> sketch1(1000, 0.01);
    CountMinSketch<uint16> sketch2(1000, 0.01);
    CountMinSketch<uint32> sketch3(1000, 0.01);

    // 300 overflows with uint8:
    EXPECT_EQ(255, sketch1.AddCount(100, 300));
    EXPECT_EQ(300, sketch2.AddCount(100, 300));
    EXPECT_EQ(300, sketch3.AddCount(100, 300));

    // 66000 overflows with uint8 and uint16:
    EXPECT_EQ(255, sketch1.AddCount(200, 66000));
    EXPECT_EQ(65535, sketch2.AddCount(200, 66000));
    EXPECT_EQ(66000, sketch3.AddCount(200, 66000));

    // 5,000,000,000 overflows with all:
    EXPECT_EQ(255, sketch1.AddCount(300, 5000000000LL));
    EXPECT_EQ(65535, sketch2.AddCount(300, 5000000000LL));
    EXPECT_EQ(4294967295LL, sketch3.AddCount(300, 5000000000LL));
}

TEST_F(CountMinSketchTest, StatisticalTest) {
    const int kNumDistinctKeys = 1000000;
    const int kNumInsertions = kNumDistinctKeys * 10;

    CountMinSketch<uint8> sketch(kNumDistinctKeys * 8 /*8 byte per key*/, 0.01);
    hash_map<uint64, int> exact_count;

    for (int i = 0; i < kNumInsertions; ++i) {
        uint64 key = rand() % kNumDistinctKeys;  // NOLINT
        exact_count[key]++;
        // If the approx count for this ley exceeded 10, take back by 10.
        if (sketch.Increment(key) >= 10) {
            sketch.AddCount(key, -10);
            exact_count[key] -= 10;
        }
    }

    double sum_counts = 0;
    double sum_error = 0;
    for (const auto& p : exact_count) {
        uint64 key = p.first;
        int exact_count = p.second;
        int approx_count = sketch.GetCount(key);
        sum_error += std::abs(exact_count - approx_count);
        sum_counts += exact_count;
    }
    double avg_error = sum_error / sum_counts;

    LOG(INFO) << "Average error = " << std::round(avg_error * 100.) << "%";
    EXPECT_LE(avg_error, .05);  // 5%
}
