#include <gtest/gtest.h>

#include <cmath>
#include <set>

#include "cpp-base/data-struct/bloom-filter/bloom_filter.h"
#include "cpp-base/util/map_util.h"

using cpp_base::BloomFilter;
using cpp_base::ContainsKey;

class BloomFilterTest : public ::testing::Test {
  public:
    BloomFilterTest() { }
    ~BloomFilterTest() { }

  protected:
    void SetUp() override { }
    void TearDown() override { }
};

TEST_F(BloomFilterTest, BasicTest) {
    BloomFilter bf(1000, 100);
    EXPECT_EQ(0, bf.NumElements());
    EXPECT_FALSE(bf.Contains(25));

    EXPECT_TRUE(bf.Insert(25));
    EXPECT_TRUE(bf.Contains(25));
    EXPECT_EQ(1, bf.NumElements());

    EXPECT_FALSE(bf.Contains(35));
    EXPECT_FALSE(bf.Insert(25));

    bf.Clear();
    EXPECT_EQ(0, bf.NumElements());
    EXPECT_FALSE(bf.Contains(25));
}

TEST_F(BloomFilterTest, StatisticalTest) {
    const int N = 1000000;
    BloomFilter bf(N * 6, N);
    std::set<int> keys;
    for (int i = 0; i < N; ++i) {
        int key = rand();  // NOLINT
        bf.Insert(key);
        keys.insert(key);
    }

    // Ensure no false negative.
    for (int x : keys) {
        EXPECT_TRUE(bf.Contains(x));
    }

    // Measure false positive:
    int false_count = 0, total_count = 0;
    for (int i = 0; i < N; ++i) {
        int key = rand();  // NOLINT
        if (ContainsKey(keys, key))
            continue;
        if (bf.Contains(key))
            ++false_count;
        ++total_count;
    }
    double fp = false_count * 100. / total_count;
    LOG(INFO) << "False positive ratio: " << fp << "%";
    CHECK_LT(fp, 10);
}
