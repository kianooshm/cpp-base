#include <glog/logging.h>
#include <gtest/gtest.h>

#include <cmath>
#include <set>

#include "cpp-base/data-struct/cuckoo-filter/cuckoo_filter.h"
#include "cpp-base/util/map_util.h"

using cpp_base::CuckooFilter;
using cpp_base::ContainsKey;

class CuckooFilterTest : public ::testing::Test {
  public:
    CuckooFilterTest() { }
    ~CuckooFilterTest() { }

  protected:
    void SetUp() override { }
    void TearDown() override { }
};

TEST_F(CuckooFilterTest, BasicTest) {
    CuckooFilter cf(10, 8);
    EXPECT_EQ(0, cf.NumElements());
    EXPECT_FALSE(cf.Contains(25));

    cf.Insert(25);
    EXPECT_TRUE(cf.Contains(25));
    EXPECT_FALSE(cf.Contains(35));
    EXPECT_EQ(1, cf.NumElements());
}

TEST_F(CuckooFilterTest, StatisticalTest) {
    const int NUM_ELEMENTS = 1000000;
    const int KEY_RANGE = 10 * NUM_ELEMENTS;

    CuckooFilter cf(NUM_ELEMENTS, 12);

    std::set<int> keys;
    for (int i = 0; i < NUM_ELEMENTS; ++i) {
        int key = rand() % KEY_RANGE;  // NOLINT
        cf.Insert(key);
        keys.insert(key);
    }

    // Ensure no false negative.
    for (int x : keys)
        EXPECT_TRUE(cf.Contains(x));

    // Delete 1/4th of the items:
    int i = 0;
    for (std::set<int>::iterator it = keys.begin(); i < NUM_ELEMENTS / 4; ++i)
        keys.erase(it++);

    // Measure false positive:
    int false_count = 0;
    int total_count = 0;
    for (int i = 0; i < KEY_RANGE; ++i) {
        int key = rand();  // NOLINT
        if (cf.Contains(key) && !ContainsKey(keys, key))
            ++false_count;
        ++total_count;
    }
    double fp = false_count * 100. / total_count;
    LOG(INFO) << "False positive ratio: " << fp << "%";
    CHECK_LT(fp, 1);
}
