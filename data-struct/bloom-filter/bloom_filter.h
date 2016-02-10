#ifndef CPP_BASE_DATA_STRUCT_BLOOM_FILTER_BLOOM_FILTER_H_
#define CPP_BASE_DATA_STRUCT_BLOOM_FILTER_BLOOM_FILTER_H_

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "cpp-base/integral_types.h"
#include "cpp-base/macros.h"

namespace cpp_base {

// Bloom Filter works as an approximate set. Items can be inserted and
// looked up, but not removed. This class is *not* thread-safe.
class BloomFilter {
  public:
    // Constructs an empty filter with the given size. The expected number of
    // elements is used to figure out the optimal number of hash functions.
    // If the actual number of elements deviates from this value, the bloom
    // filter doesn't break; it just becomes sub-optimal.
    BloomFilter(int64 bit_size, int64 expected_num_elements);

    ~BloomFilter();

    // Returns true if the given key is inserted, false if it already existed.
    bool Insert(uint64 key);

    // Returns whether the given key exists in the filter.
    bool Contains(uint64 key) const;

    // Clears the filter.
    void Clear();

    const uint8* RawData() const { return data_; }
    int64 BitSize() const { return bit_size_; }
    int64 NumElements() const { return num_inserts_; }

    void SaveToFile(const std::string& path) { /*TODO(kianoosh)*/ }
    void LoadFromFile(const std::string& path) { /*TODO(kianoosh)*/ }

    static inline bool IsPowerOf2(int64 x) { return (x & (x - 1)) == 0 || x == 1; }

  private:
    const int64 byte_size_;         // Size in bytes
    const int64 bit_size_;          // Size in bits
    uint8* data_;                   // The underlying bit vector
    int64 num_inserts_ = 0;         // Num elements inserted so far

    // These are used for logging a warning if the Bloom Filter is over-used.
    const int64 expected_num_elements_;
    int64 log_regulator_ = 0;

    // Random keys used for hashing.
    std::vector<uint64> hash_keys_;

    DISALLOW_COPY_AND_ASSIGN(BloomFilter);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_BLOOM_FILTER_BLOOM_FILTER_H_
