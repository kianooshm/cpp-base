// Based on https://github.com/efficient/cuckoofilter by Efficient Computing at Carnegie Mellon

#ifndef CPP_BASE_DATA_STRUCT_CUCKOO_FILTER_CUCKOO_FILTER_H_
#define CPP_BASE_DATA_STRUCT_CUCKOO_FILTER_CUCKOO_FILTER_H_

#include <string>

#include "cpp-base/data-struct/cuckoo-filter/single_table.h"
#include "cpp-base/data-struct/cuckoo-filter/util.h"
#include "cpp-base/hash/fingerprint2011.h"
#include "cpp-base/hash/hash.h"
#include "cpp-base/macros.h"

namespace cpp_base {

// maximum number of cuckoo kicks before claiming failure
const int64 kMaxCuckooCount = 500;

class CuckooFilter {
  public:
    #if 0
    // Deprecated constructor.
    explicit CuckooFilter(int64 bit_size) : bits_per_element_(16), num_elements_(0) {
        static const int64 kAssociativity = 4;

        int64 bytes_per_bucket = (bits_per_element_ * kAssociativity + 7) >> 3;
        double __num_buckets = bit_size / 8. / bytes_per_bucket;
        int64 num_buckets = upperpower2(std::ceil(__num_buckets));

        last_victim_.used = false;
        table_ = new SingleTable(num_buckets, kAssociativity, bits_per_element_);
    }
    #endif

    // bits_per_item: how many bits each item is hashed into.
    CuckooFilter(int64 expected_num_elements, int bits_per_element)
            : bits_per_element_(bits_per_element),
              num_elements_(0),
              expected_num_elements_(expected_num_elements) {
        static const int64 kAssociativity = 4;

        double __num_buckets = expected_num_elements * 1. / kAssociativity;
        int64 num_buckets = upperpower2(std::ceil(__num_buckets));
        double frac = __num_buckets / num_buckets;
        if (frac >= 0.96)
            num_buckets *= 2;

        last_victim_.used = false;
        table_ = new SingleTable(num_buckets, kAssociativity, bits_per_element);
    }

    ~CuckooFilter() {
        delete table_;
    }

    // Add an item to the filter. Returns true if successfully inserted, false if not enough space.
    // Note: the caller should try to avoid re-insertion.
    bool Insert(uint64 key) {
        if (last_victim_.used)
            return false;  // Not enough space

        int64 bucket_index;
        uint32 tag;
        GetIndexAndTag(key, &bucket_index, &tag);
        bool ret = AddInternal(bucket_index, tag);

        // If we went beyond the expected size, log a warning, but only at a regulated rate.
        if (num_elements_ > expected_num_elements_ && is_power_of_2(++log_regulator_)) {
            LOG(WARNING) << "CF insertions (" << num_elements_ << ") exceeded the expected max ("
                         << expected_num_elements_ << "). Accuracy will degrade. Need more memory.";
        }

        return ret;
    }

    // Report if the item is inserted, with false positive rate.
    bool Contains(uint64 key) {
        int64 bucket_index1;
        int64 bucket_index2;
        uint32 tag;
        GetIndexAndTag(key, &bucket_index1, &tag);

        bucket_index2 = AltIndex(bucket_index1, tag);
        CHECK_EQ(bucket_index1, AltIndex(bucket_index2, tag));

        if (last_victim_.used && tag == last_victim_.tag && (bucket_index1 == last_victim_.index ||
                                                             bucket_index2 == last_victim_.index)) {
            return true;
        }
        if (table_->FindTagInBuckets(bucket_index1, bucket_index2, tag)) {
            return true;
        }
        return false;
    }

    // Delete an key from the filter.
    // NOTE: Make sure the item exists, before calling this function.
    bool Delete(uint64 key) {
        int64 bucket_index1;
        int64 bucket_index2;
        uint32 tag;
        GetIndexAndTag(key, &bucket_index1, &tag);

        bucket_index2 = AltIndex(bucket_index1, tag);
        CHECK_EQ(bucket_index1, AltIndex(bucket_index2, tag));

        if (table_->DeleteTagFromBucket(bucket_index1, tag)) {
            --num_elements_;
            goto TryEliminateVictim;
        } else if (table_->DeleteTagFromBucket(bucket_index2, tag)) {
            --num_elements_;
            goto TryEliminateVictim;
        } else if (last_victim_.used && tag == last_victim_.tag &&
                 (bucket_index1 == last_victim_.index || bucket_index2 == last_victim_.index)) {
            last_victim_.used = false;
            return true;
        } else {
            return false;
        }

      TryEliminateVictim:
        if (last_victim_.used) {
            last_victim_.used = false;
            int64 i = last_victim_.index;
            uint32 tag = last_victim_.tag;
            AddInternal(i, tag);
        }
        return true;
    }

    bool Insert(const std::string& str)   { return Insert(Fingerprint2011(str));   }
    bool Contains(const std::string& str) { return Contains(Fingerprint2011(str)); }
    bool Delete(const std::string& str)   { return Delete(Fingerprint2011(str));   }

    void Clear() {
        table_->Clear();
        num_elements_ = 0;
        last_victim_ = LastVictim();
    }

    // Number of inserted items so far.
    int64 NumElements() const { return num_elements_; }

    // Size of the filter in bytes.
    int64 SizeInBytes() const { return table_->SizeInBytes(); }

    // Summary info.
    std::string Info() const {
        std::stringstream ss;
        ss << "CuckooFilter Status:\n"
           << "\t\t" << table_->Info() << "\n"
           << "\t\tKeys stored: " << NumElements() << "\n"
           << "\t\tLoad facotr: " << LoadFactor() << "\n"
           << "\t\tHashtable size: " << (table_->SizeInBytes() >> 10)
           << " KB\n";
        if (NumElements() > 0) {
            ss << "\t\tbit/key:   " << BitsPerItem() << "\n";
        } else {
            ss << "\t\tbit/key:   N/A\n";
        }
        return ss.str();
    }

  private:
    inline int64 BucketIndexHash(uint32 val) const {
        return val % table_->NumBuckets();
    }

    inline uint32 TagHash(uint32 val) const {
        uint32 tag;
        tag = val & ((1ULL << bits_per_element_) - 1);
        tag += (tag == 0);
        return tag;
    }

    inline void GetIndexAndTag(uint64 key, int64* bucket_index, uint32* tag) {
        // A re-hash is necessary:
        uint64 h = Hash64NumWithSeed(key, 0xa5b85c5e198ed849ULL /*some big prime number*/);
        *bucket_index = BucketIndexHash((uint32) (h >> 32));
        *tag = TagHash((uint32) (h & 0xFFFFFFFF));
    }

    inline int64 AltIndex(int64 index, uint32 tag) const {
        // NOTE(binfan): originally we use:
        // index ^ HashUtil::BobHash((const void*) (&tag), 4)) & table_->INDEXMASK;
        // now doing a quick-n-dirty way:
        // 0x5bd1e995 is the hash constant from MurmurHash2
        return BucketIndexHash((uint32) (index ^ (tag * 0x5bd1e995)));
    }

    bool AddInternal(const int64 bucket_index, const uint32 tag) {
        int64 curindex = bucket_index;
        uint32 curtag = tag;
        uint32 oldtag;

        for (uint32 count = 0; count < kMaxCuckooCount; count++) {
            bool kickout = count > 0;
            oldtag = 0;
            if (table_->InsertTagToBucket(curindex, curtag, kickout, oldtag)) {
                ++num_elements_;
                return true;
            }
            if (kickout) {
                curtag = oldtag;
            }
            curindex = AltIndex(curindex, curtag);
        }

        last_victim_.index = curindex;
        last_victim_.tag = curtag;
        last_victim_.used = true;
        return true;
    }

    // load factor is the fraction of occupancy.
    double LoadFactor() const { return 1.0 * NumElements()  / table_->SizeInTags(); }

    double BitsPerItem() const { return 8.0 * table_->SizeInBytes() / NumElements(); }

    const int bits_per_element_;
    SingleTable* table_;  // Storage of items
    int64 num_elements_;  // Number of items stored

    struct LastVictim {
        LastVictim() : index(0), tag(0), used(false) {}
        int64 index;
        uint32 tag;
        bool used;
    };
    LastVictim last_victim_;

    // These are used for logging a warning if the Cuckoo Filter is over-used.
    const int64 expected_num_elements_;
    int64 log_regulator_ = 0;

    DISALLOW_COPY_AND_ASSIGN(CuckooFilter);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_CUCKOO_FILTER_CUCKOO_FILTER_H_
