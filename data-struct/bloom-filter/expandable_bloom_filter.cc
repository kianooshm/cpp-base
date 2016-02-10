#include "cpp-base/data-struct/bloom-filter/expandable_bloom_filter.h"

#include <glog/logging.h>

namespace cpp_base {

ExpandableBloomFilter::ExpandableBloomFilter(int64 initial_bit_size, int64 initial_cutoff_size) {
    BfInstance instance;
    instance.bf_.reset(new BloomFilter(initial_bit_size, initial_cutoff_size));
    instance.cutoff_size_ = initial_cutoff_size;
    instances_.push_back(std::move(instance));
}

bool ExpandableBloomFilter::Insert(uint64 key) {
    // If the key exists in any of the bloom filters, do not insert.
    if (Contains(key))
        return false;

    // Insert in the last filter.
    BfInstance* last = &instances_.back();
    CHECK_LE(last->bf_->NumElements(), last->cutoff_size_);

    // Is the last filter full?
    if (last->bf_->NumElements() == last->cutoff_size_) {
        // Create a new filter with a size equal to all existing filters' size.
        int64 capacity = 0;
        int64 bit_size = 0;
        for (const BfInstance& b : instances_) {
            capacity += b.cutoff_size_;
            bit_size += b.bf_->BitSize();
        }
        BfInstance instance;
        instance.cutoff_size_ = capacity;
        instance.bf_.reset(new BloomFilter(bit_size, capacity));
        instances_.push_back(std::move(instance));
        last = &instances_.back();
    }

    // Insert in the last filter.
    CHECK(last->bf_->Insert(key)) << key;
    ++num_inserts_;
    return true;
}

bool ExpandableBloomFilter::Contains(uint64 key) const {
    // Search the list of bloom filters from last to first, because larger filters are
    // exponentially more likely to get a hit.
    for (auto it = instances_.rbegin(); it != instances_.rend(); ++it) {
        if (it->bf_->Contains(key))
            return true;
    }
    return false;
}

void ExpandableBloomFilter::Clear() {
    instances_.resize(1);
    instances_.front().bf_->Clear();
    num_inserts_ = 0;
}

int64 ExpandableBloomFilter::BitSize() const {
    int64 sum = 0;
    for (const BfInstance& instance : instances_) {
        sum += instance.bf_->BitSize();
    }
    return sum;
}

}  // namespace cpp_base
