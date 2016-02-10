#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "cpp-base/data-struct/lru_map.h"

using cpp_base::LruMap;
using std::pair;
using std::string;
using std::vector;

class LruMapTest : public ::testing::Test {
  public:
    LruMapTest() { }
    ~LruMapTest() { }

  protected:
    void SetUp() override { }
    void TearDown() override { }

    template <class K, class V>
    static void CheckEntriesOrder(const LruMap<K, V>& m, const vector<K>& keys,
                                  const vector<V>& values, int line_no) {
        CHECK_EQ(keys.size(), values.size()) << line_no;
        vector<pair<K, V>> l = m.RawList();
        EXPECT_EQ(m.Size(), keys.size()) << line_no;
        EXPECT_EQ(l.size(), keys.size()) << line_no;
        // Compare item by item:
        for (int i = 0; i < l.size(); ++i) {
            EXPECT_EQ(keys[i], l[i].first) << line_no << "; " << i;
            EXPECT_EQ(values[i], l[i].second) << line_no << "; " << i;
        }
        m.CheckInternalCorrectness();
    }

    template <class K, class V>
    static void CheckKeyValueInMap(LruMap<K, V>* m, const K& key,
                                   bool key_exists, bool get_with_touch,
                                   const V& value, int line_no) {
        V v;
        if (key_exists) {
            EXPECT_TRUE(m->Contains(key)) << line_no;
            if (get_with_touch) {
                EXPECT_TRUE(m->GetWithTouch(key, &v)) << line_no;
            } else {
                EXPECT_TRUE(m->GetWithoutTouch(key, &v)) << line_no;
            }
            EXPECT_EQ(value, v) << line_no;
        } else {
            EXPECT_FALSE(m->Contains(key)) << line_no;
            if (get_with_touch) {
                EXPECT_FALSE(m->GetWithTouch(key, &v)) << line_no;
            } else {
                EXPECT_FALSE(m->GetWithoutTouch(key, &v)) << line_no;
            }
        }
    }
};

TEST_F(LruMapTest, BasicTest) {
    LruMap<string, int> m;
    int x;
    string s;
    // Check initial conditions:
    EXPECT_TRUE(m.Empty());
    EXPECT_EQ(0, m.Size());
    EXPECT_FALSE(m.Contains(""));
    EXPECT_FALSE(m.GetWithoutTouch("", &x));
    EXPECT_FALSE(m.GetWithTouch("", &x));
    EXPECT_FALSE(m.Touch(""));
    EXPECT_FALSE(m.OldestKey(&s));
    EXPECT_FALSE(m.OldestValue(&x));
    EXPECT_FALSE(m.EvictOldest(&x));
    // This will check the internal consistency of LruMap's data structures.
    m.CheckInternalCorrectness();

    // Insert {one->1} and {two->2}:
    m.Put("one", 1);
    EXPECT_FALSE(m.Empty());
    EXPECT_EQ(1, m.Size());
    m.Put("two", 2);
    EXPECT_FALSE(m.Empty());
    EXPECT_EQ(2, m.Size());
    m.CheckInternalCorrectness();

    // Check map contents:
    CheckKeyValueInMap(&m, string("one"), true, false, 1, __LINE__);
    CheckKeyValueInMap(&m, string("two"), true, false, 2, __LINE__);
    CheckKeyValueInMap(&m, string("xxx"), false, false, 0, __LINE__);

    // There should be no difference between Get{With|Without}Touch():
    CheckKeyValueInMap(&m, string("one"), true, true, 1, __LINE__);
    CheckKeyValueInMap(&m, string("two"), true, true, 2, __LINE__);
    CheckKeyValueInMap(&m, string("xxx"), false, true, 0, __LINE__);

    // Erase {one->1}:
    EXPECT_TRUE(m.Erase("one", &x));
    EXPECT_EQ(1, x);
    EXPECT_EQ(1, m.Size());
    CheckKeyValueInMap(&m, string("one"), false, false, 1, __LINE__);

    // Try erasing a non-existent item:
    EXPECT_FALSE(m.Erase("one", &x));

    // {two->2} should still be there:
    CheckKeyValueInMap(&m, string("two"), true, false, 2, __LINE__);
}

TEST_F(LruMapTest, TestListOrder) {
    LruMap<string, int> m(100);
    m.Put("one",   1);
    m.Put("three", 3);
    m.Put("six",   6);
    m.Put("four",  4);

    // Items should be ordered by insertion time: 1 3 6 4.
    CheckEntriesOrder(m, vector<string>{"one", "three", "six", "four"},
                         vector<int>{1, 3, 6, 4}, __LINE__);
    CheckKeyValueInMap(&m, string("one"),   true, false, 1, __LINE__);
    CheckKeyValueInMap(&m, string("three"), true, false, 3, __LINE__);
    CheckKeyValueInMap(&m, string("six"),   true, false, 6, __LINE__);
    CheckKeyValueInMap(&m, string("four"),  true, false, 4, __LINE__);

    // Touch 3: items should then be ordered as: 1 6 4 3.
    EXPECT_TRUE(m.Touch("three"));
    CheckEntriesOrder(m, vector<string>{"one", "six", "four", "three"},
                         vector<int>{1, 6, 4, 3}, __LINE__);

    // Check LruMap::OldestKey() and LruMap::OldestValue().
    int x;
    string s;
    EXPECT_TRUE(m.OldestKey(&s));
    EXPECT_TRUE(m.OldestValue(&x));
    EXPECT_EQ("one", s);
    EXPECT_EQ(1, x);

    // Touch 1: items should then be ordered as: 6 4 3 1.
    EXPECT_TRUE(m.Touch("one"));
    CheckEntriesOrder(m, vector<string>{"six", "four", "three", "one"},
                         vector<int>{6, 4, 3, 1}, __LINE__);

    // Touch 1 again: the order shouldn't change.
    EXPECT_TRUE(m.Touch("one"));
    CheckEntriesOrder(m, vector<string>{"six", "four", "three", "one"},
                         vector<int>{6, 4, 3, 1}, __LINE__);

    // GetWithoutTouch() shouldn't affect things:
    EXPECT_TRUE(m.GetWithoutTouch("four", &x));
    EXPECT_TRUE(m.GetWithoutTouch("six", &x));
    CheckEntriesOrder(m, vector<string>{"six", "four", "three", "one"},
                         vector<int>{6, 4, 3, 1}, __LINE__);

    // Check LruMap::OldestKey() and LruMap::OldestValue().
    EXPECT_TRUE(m.OldestKey(&s));
    EXPECT_TRUE(m.OldestValue(&x));
    EXPECT_EQ("six", s);
    EXPECT_EQ(6, x);

    // Doing a Put() of an existing item is equivalent to touch.
    // Current list order is: 6 4 3 1. Put 3 and expect 6 4 1 3.
    EXPECT_TRUE(m.Put("three", 3));
    CheckEntriesOrder(m, vector<string>{"six", "four", "one", "three"},
                         vector<int>{6, 4, 1, 3}, __LINE__);

    // Putting 3 again shouldn't change the order:
    EXPECT_TRUE(m.Put("three", 3));
    CheckEntriesOrder(m, vector<string>{"six", "four", "one", "three"},
                         vector<int>{6, 4, 1, 3}, __LINE__);

    // Putting 6 should:
    EXPECT_TRUE(m.Put("six", 6));
    CheckEntriesOrder(m, vector<string>{"four", "one", "three", "six"},
                         vector<int>{4, 1, 3, 6}, __LINE__);
}

TEST_F(LruMapTest, TestEviction) {
    LruMap<string, int> m;
    m.SetCapacity(3);
    m.Put("one",   1);
    m.Put("two",   2);
    m.Put("three", 3);
    CheckEntriesOrder(m, vector<string>{"one", "two", "three"},
                         vector<int>{1, 2, 3}, __LINE__);

    // New insertions will evict 1. New contents will be: 2 3 4
    m.Put("four", 4);
    EXPECT_FALSE(m.Contains("one"));
    CheckEntriesOrder(m, vector<string>{"two", "three", "four"},
                         vector<int>{2, 3, 4}, __LINE__);

    // Insertion of an existing item shouldn't get anything evicted:
    m.Put("three", 3);
    CheckEntriesOrder(m, vector<string>{"two", "four", "three"},
                         vector<int>{2, 4, 3}, __LINE__);

    // Put 1; get 2 evicted. New order: 3 4 1.
    m.Put("one", 1);
    CheckEntriesOrder(m, vector<string>{"four", "three", "one"},
                         vector<int>{4, 3, 1}, __LINE__);
    // Put 1 again. Nothing should change.
    m.Put("one", 1);
    CheckEntriesOrder(m, vector<string>{"four", "three", "one"},
                         vector<int>{4, 3, 1}, __LINE__);

    // Erase 3. New order: 4 1. Then, new insertion shouldn't evict anything.
    m.Erase("three");
    CheckEntriesOrder(m, vector<string>{"four", "one"},
                         vector<int>{4, 1}, __LINE__);
    // Insert 0: 4 1 0.
    m.Put("zero", 0);
    CheckEntriesOrder(m, vector<string>{"four", "one", "zero"},
                         vector<int>{4, 1, 0}, __LINE__);
    // Inserting 7 will get 4 evicted.
    m.Put("seven", 7);
    CheckEntriesOrder(m, vector<string>{"one", "zero", "seven"},
                         vector<int>{1, 0, 7}, __LINE__);

    // Manually evict the oldest item, which would be 1. New order: 0 7.
    int x;
    m.EvictOldest(&x);
    EXPECT_EQ(1, x);
    CheckEntriesOrder(m, vector<string>{"zero", "seven"},
                         vector<int>{0, 7}, __LINE__);
    // Keep evicting:
    m.EvictOldest(&x);  // 0
    EXPECT_EQ(0, x);
    CheckEntriesOrder(m, vector<string>{"seven"}, vector<int>{7}, __LINE__);
    m.EvictOldest(&x);  // 7
    EXPECT_EQ(7, x);
    EXPECT_TRUE(m.Empty());
}
