#include <gtest/gtest.h>

#include <cmath>
#include <set>

#include "cpp-base/data-struct/bloom-filter/expandable_bloom_filter.h"
#include "cpp-base/util/map_util.h"

using cpp_base::ContainsKey;
using cpp_base::ExpandableBloomFilter;

class ExpandableBloomFilterTest : public ::testing::Test {
  public:
    ExpandableBloomFilterTest() { }
    ~ExpandableBloomFilterTest() { }

  protected:
    void SetUp() override { }
    void TearDown() override { }
};

TEST_F(ExpandableBloomFilterTest, BasicTest) {
    ExpandableBloomFilter bf(1000, 100);
    EXPECT_EQ(1000, bf.BitSize());
    EXPECT_EQ(0, bf.NumInserts());
    EXPECT_FALSE(bf.Contains(25));

    EXPECT_TRUE(bf.Insert(25));
    EXPECT_TRUE(bf.Contains(25));
    EXPECT_EQ(1, bf.NumInserts());

    EXPECT_FALSE(bf.Contains(35));
    EXPECT_FALSE(bf.Insert(25));

    bf.Clear();
    EXPECT_EQ(0, bf.NumInserts());
    EXPECT_FALSE(bf.Contains(25));

#if 0
    // Insert 100 elements. Since there is a slight chance of false positive, we may need to go a
    // bit beyond 100 to actually have 100 successful insertions.
    int i;
    for (i = 0; bf.NumInserts() < 100; ++i)
        bf.Insert(i);

    // The filter shouldn't expand yet.
    EXPECT_EQ(1000, bf.BitSize());

    // The next insertion triggers an expansion. Again, given the slight chance of false positive,
    // we may need to try more than one insertion.
    while (!bf.Insert(++i)) {}

    // The filter should have expanded.
    EXPECT_EQ(3000, bf.BitSize());
#else
    // Insert 100 elements.
    for (int i = 1; i <= 100; ++i)
        bf.Insert(i);

    // The filter shouldn't expand yet.
    EXPECT_EQ(1000, bf.BitSize());

    // The next insertion triggers an expansion. Given the slight chance of false positive, we may
    // need to try more than one insertion.
    for (int i = 101; !bf.Insert(i); ++i) {}

    // The filter should have expanded.
    EXPECT_EQ(2000, bf.BitSize());
#endif


    // Clearing the filter should revert it back to original state.
    bf.Clear();
    EXPECT_EQ(1000, bf.BitSize());
    EXPECT_EQ(0, bf.NumInserts());
}

TEST_F(ExpandableBloomFilterTest, StatisticalTest) {
    ExpandableBloomFilter bf(100 * 2 * 8, 100);  // 2 bytes per element

    const int64 N = 100000;
    std::set<uint64> keys;
    for (int64 i = 0; i < N; ++i) {
        uint64 key = rand();  // NOLINT
        bf.Insert(key);
        keys.insert(key);
    }

    // Ensure no false negative.
    for (uint64 x : keys) {
        EXPECT_TRUE(bf.Contains(x));
    }

    // Measure false positive:
    int64 false_positive = 0, true_negative = 0;
    for (int64 i = 0; i < 3 * N; ++i) {
        uint64 key = rand();  // NOLINT
        if (bf.Contains(key)) {
            if (!ContainsKey(keys, key))
                ++false_positive;
        } else {
            CHECK(!ContainsKey(keys, key)) << key;
            ++true_negative;
        }
    }
    double fp = false_positive * 100. / (false_positive + true_negative);
    LOG(INFO) << "False positive ratio: " << fp << "%";
    CHECK_LE(fp, 1);
}
