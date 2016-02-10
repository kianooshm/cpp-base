#include "cpp-base/data-struct/bloom-filter/bloom_filter.h"

#include <glog/logging.h>
#include <algorithm>
#include <random>
#include "cpp-base/hash/hash.h"

namespace cpp_base {

BloomFilter::BloomFilter(int64 bit_size, int64 expected_num_elements)
        : byte_size_((bit_size - 1) / 8 + 1),
          bit_size_(byte_size_ * 8),
          expected_num_elements_(expected_num_elements) {
    data_ = new uint8[byte_size_];
    memset(data_, 0, byte_size_);

    // Optimal number of hash functions:
    int64 num_hashes = std::round(bit_size_ * 1. / expected_num_elements * std::log(2));
    if (num_hashes == 0)
        num_hashes = 1;
    else if (num_hashes > 8)  // More than 8 hash functions is unnecessary and only time consuming
        num_hashes = 8;

    std::mt19937_64 rand_gen(1000);
    for (int64 i = 0; i < num_hashes; ++i)
        hash_keys_.push_back(rand_gen());

    // LOG(INFO) only for large BFs.
    if (bit_size >= 800000000) {
        // Expected false positive probability:
        double temp = std::pow(1 - 1. / bit_size_, num_hashes * expected_num_elements);
        double false_pos_pr = std::pow(1 - temp, num_hashes);
        LOG(INFO) << "Initialized bloom filter of " << bit_size_ << " bits for "
                  << expected_num_elements << " elements => num hash functions: "
                  << num_hashes << "; expected pr% of false positive: " << false_pos_pr * 100;
    }
}

BloomFilter::~BloomFilter() {
    delete[] data_;
}

bool BloomFilter::Insert(uint64 key) {
    bool inserted = false;
    for (int64 i = 0; i < hash_keys_.size(); ++i) {
        uint64 mix = Hash64NumWithSeed(key, hash_keys_[i]);
        int64 index = mix % bit_size_;
        int64 byte = index / 8;
        int64 bit = index % 8;
        uint8 prev = data_[byte];
        data_[byte] |= 1 << bit;
        if (prev != data_[byte])
            inserted = true;
    }
    //if (inserted)
    ++num_inserts_;

    // If we went beyond the expected size, log a warning, but only at a regulated rate.
    if (num_inserts_ > expected_num_elements_ && IsPowerOf2(++log_regulator_)) {
        LOG(WARNING) << "BF insertions (" << num_inserts_ << ") exceeded the expected maximum ("
                     << expected_num_elements_ << "). Accuracy will degrade. Need more memory.";
    }

    return inserted;
}

bool BloomFilter::Contains(uint64 key) const {
    for (int64 i = 0; i < hash_keys_.size(); ++i) {
        uint64 mix = Hash64NumWithSeed(key, hash_keys_[i]);
        int64 index = mix % bit_size_;
        int64 byte = index / 8;
        int64 bit = index % 8;
        if (!(data_[byte] & (1 << bit)))
            return false;
    }
    return true;
}

void BloomFilter::Clear() {
    memset(data_, 0, byte_size_);
    num_inserts_ = 0;
}

}  // namespace cpp_base
