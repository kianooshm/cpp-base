#ifndef CPP_BASE_DATA_STRUCT_BLOOM_FILTER_EXPANDABLE_BLOOM_FILTER_H_
#define CPP_BASE_DATA_STRUCT_BLOOM_FILTER_EXPANDABLE_BLOOM_FILTER_H_

#include <memory>
#include <vector>

#include "cpp-base/data-struct/bloom-filter/bloom_filter.h"

namespace cpp_base {

// A wrapper on Bloom Filter that allows to create a small bloom filter and expand as necessary.
// First, we create an initial BF for e.g. 1000 elements. Upon insertion of the 1001st element,
// a new BF is created for 1000 elements so the total size increases to 2000. On the 2001st element,
// the next BF is created for 2000 elements, and so on.
// Lookup is done in O(log(n/initial_cutoff_size)). So is insertion, because it does a lookup first.
// Note that the false positive ratio of such a filter is lower than a normal Bloom Filter:
// Pr(false pos) = Pr(false pos in BF 1 | false pos in BF 2 | ...).
class ExpandableBloomFilter {
  public:
    // initial_bit_size: capacity of the BF in number of bits.
    // initial_cutoff_size: after this many insertions, double the size.
    // Num bits per element is: initial_bit_size / initial_cutoff_size.
    ExpandableBloomFilter(int64 initial_bit_size, int64 initial_cutoff_size);

    // Returns true if the given key is inserted, false if it already existed.
    bool Insert(uint64 key);

    // Returns whether the given key exists in the filter.
    bool Contains(uint64 key) const;

    // Clears the filter.
    void Clear();

    int64 BitSize() const;

    int64 NumInserts() const { return num_inserts_; }

  private:
    struct BfInstance {
        std::unique_ptr<BloomFilter> bf_;
        int64 cutoff_size_;
    };
    std::vector<BfInstance> instances_;
    int64 num_inserts_ = 0;
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_BLOOM_FILTER_EXPANDABLE_BLOOM_FILTER_H_
