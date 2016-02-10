// Based on https://github.com/efficient/cuckoofilter by Efficient Computing at Carnegie Mellon

#ifndef CPP_BASE_DATA_STRUCT_CUCKOO_FILTER_SINGLE_TABLE_H_
#define CPP_BASE_DATA_STRUCT_CUCKOO_FILTER_SINGLE_TABLE_H_

#include <glog/logging.h>
#include <xmmintrin.h>

#include <random>
#include <sstream>

#include "cpp-base/data-struct/cuckoo-filter/util.h"
#include "cpp-base/integral_types.h"

#define ASSERT_LITTLE_ENDIAN() {                       \
    int n = 1;                                         \
    char* ptr = reinterpret_cast<char*>(&n);           \
    CHECK(*ptr == 1) << "System is not little endian"; \
}

namespace cpp_base {

// the most naive table implementation: one huge bit array
class SingleTable {
  public:
    SingleTable(int64 num_buckets, int tags_per_bucket, int bits_per_tag)
            : num_buckets_(num_buckets),
              tags_per_bucket_(tags_per_bucket),
              bytes_per_bucket_((bits_per_tag * tags_per_bucket + 7) >> 3),
              bits_per_tag_(bits_per_tag),
              tag_mask_((1ULL << bits_per_tag_) - 1),
              data_(new uint8[num_buckets * bytes_per_bucket_ + 8]),
              rand_gen_(12345678) {
        CHECK_GT(num_buckets_, 0);
        CHECK_GT(tags_per_bucket_, 0);
        CHECK_GT(bytes_per_bucket_, 0);
        CHECK_GT(bits_per_tag_, 0);
        memset(data_, 0, num_buckets * bytes_per_bucket_);

        LOG(INFO) << "Inited a " << num_buckets << "x" << bytes_per_bucket_ << " Cuckoo Filter ("
                  << (num_buckets * 1. * bytes_per_bucket_ / 1024 / 1024 / 1024) << " GB)";
    }

    ~SingleTable() {
        delete [] data_;
    }

    void Clear() {
        memset(data_, 0, num_buckets_ * bytes_per_bucket_);
    }

    int64 SizeInBytes() const { return bytes_per_bucket_ * num_buckets_; }

    int64 SizeInTags() const { return tags_per_bucket_ * num_buckets_; }

    int64 NumBuckets() const { return num_buckets_; }

    std::string Info() const  {
        std::stringstream ss;
        ss << "SingleHashtable with tag size: " << bits_per_tag_ << " bits \n";
        ss << "\t\tAssociativity: " << tags_per_bucket_ << "\n";
        ss << "\t\tTotal # of rows: " << num_buckets_ << "\n";
        ss << "\t\tTotal # slots: " << SizeInTags() << "\n";
        return ss.str();
    }

    // read tag from pos(i,j)
    inline uint32 ReadTag(const int64 i, const int64 j) const {
        const uint8* p = data_ + i * bytes_per_bucket_;
        uint32 tag = 0;

        // The following code only works for little-endian.
        ASSERT_LITTLE_ENDIAN();

        if (bits_per_tag_ == 2) {
            tag = *p >> (j * 2);
        }
        else if (bits_per_tag_ == 4) {
            p += (j >> 1);
            tag = *p >> ((j & 1) << 2);
        }
        else if (bits_per_tag_ == 8) {
            p += j;
            tag = *p;
        }
        else if (bits_per_tag_ == 12) {
            p += j + (j >> 1);
            tag = *((uint16*) p) >> ((j & 1) << 2);
        }
        else if (bits_per_tag_ == 16) {
            p += (j << 1);
            tag = *((uint16*) p);
        }
        else if (bits_per_tag_ == 32) {
            tag = ((uint32*) p)[j];
        } else {
            LOG(FATAL) << "Unsupported bits_per_tag " << bits_per_tag_;
        }
        return tag & tag_mask_;
    }

    // write tag to pos(i,j)
    inline void  WriteTag(const int64 i, const int64 j, const uint32 __tag) {
        uint8* p = data_ + i * bytes_per_bucket_;
        uint32 tag = __tag & tag_mask_;

        // The following code only works for little-endian.
        ASSERT_LITTLE_ENDIAN();

        if (bits_per_tag_ == 2) {
            *p |= tag << (2 * j);
        }
        else if (bits_per_tag_ == 4) {
            p += (j >> 1);
            if ( (j & 1) == 0) {
                *p  &= 0xf0;
                *p  |= tag;
            }
            else {
                *p  &= 0x0f;
                *p  |= (tag << 4);
            }
        }
        else if (bits_per_tag_ == 8) {
            p[j] =  tag;
        }
        else if (bits_per_tag_ == 12) {
            p += (j + (j >> 1));
            if ( (j & 1) == 0) {
                ((uint16*) p)[0] &= 0xf000;
                ((uint16*) p)[0] |= tag;
            }
            else {
                ((uint16*) p)[0] &= 0x000f;
                ((uint16*) p)[0] |= (tag << 4);
            }
        }
        else if (bits_per_tag_ == 16) {
            ((uint16*) p)[j] = tag;
        }
        else if (bits_per_tag_ == 32) {
            ((uint32*) p)[j] = tag;
        }
    }

    inline bool FindTagInBuckets(const int64 i1,
                                 const int64 i2,
                                 const uint32 tag) const {
        const uint8* p1 = data_ + i1 * bytes_per_bucket_;
        const uint8* p2 = data_ + i2 * bytes_per_bucket_;

        uint64 v1 =  *((uint64*) p1);
        uint64 v2 =  *((uint64*) p2);

        // caution: unaligned access & assuming little endian
        if (bits_per_tag_ == 4 && tags_per_bucket_ == 4) {
            return hasvalue4(v1, tag) || hasvalue4(v2, tag);
        }
        else if (bits_per_tag_ == 8 && tags_per_bucket_ == 4) {
            return hasvalue8(v1, tag) || hasvalue8(v2, tag);
        }
        else if (bits_per_tag_ == 12 && tags_per_bucket_ == 4) {
            return hasvalue12(v1, tag) || hasvalue12(v2, tag);
        }
        else if (bits_per_tag_ == 16 && tags_per_bucket_ == 4) {
            return hasvalue16(v1, tag) || hasvalue16(v2, tag);
        }
        else {
            for (int64 j = 0; j < tags_per_bucket_; j++)
                if ((ReadTag(i1, j) == tag) || (ReadTag(i2,j) == tag))
                    return true;
            return false;
        }
    }

    inline bool FindTagInBucket(const int64 i, const uint32 tag) const {
        // Caution: unaligned access & assuming little endian.
        const uint8* p = data_ + i * bytes_per_bucket_;
        uint64 val = *(uint64*)p;
        ASSERT_LITTLE_ENDIAN();
        if (bits_per_tag_ == 4 && tags_per_bucket_ == 4) {
            return hasvalue4(val, tag);
        }
        else if (bits_per_tag_ == 8 && tags_per_bucket_ == 4) {
            return hasvalue8(val, tag);
        }
        else if (bits_per_tag_ == 12 && tags_per_bucket_ == 4) {
            return hasvalue12(val, tag);
        }
        else if (bits_per_tag_ == 16 && tags_per_bucket_ == 4) {
            return hasvalue16(val, tag);
        }
        else {
            for (int64 j = 0; j < tags_per_bucket_; j++) {
                if (ReadTag(i, j) == tag)
                    return true;
            }
            return false;
        }
    }

    inline bool DeleteTagFromBucket(const int64 i, const uint32 tag) {
        for (int64 j = 0; j < tags_per_bucket_; j++) {
            if (ReadTag(i, j) == tag) {
                CHECK(FindTagInBucket(i, tag));
                WriteTag(i, j, 0);
                return true;
            }
        }
        return false;
    }

    inline bool InsertTagToBucket(const int64 i,  const uint32 tag,
                                  const bool kickout, uint32& oldtag) {
        for (int64 j = 0; j < tags_per_bucket_; j++) {
            if (ReadTag(i, j) == 0) {
                WriteTag(i, j, tag);
                return true;
            }
        }
        if (kickout) {
            int64 r = rand_gen_() % tags_per_bucket_;
            oldtag = ReadTag(i, r);
            WriteTag(i, r, tag);
        }
        return false;
    }

    inline int64 NumTagsInBucket(const int64 i) {
        int64 num = 0;
        for (int64 j = 0; j < tags_per_bucket_; j++) {
            if (ReadTag(i, j) != 0)
                ++num;
        }
        return num;
    }

  private:
    const int64 num_buckets_;
    const int64 tags_per_bucket_;
    const int64 bytes_per_bucket_;
    const int bits_per_tag_;
    const uint32 tag_mask_;
    uint8* data_;
    std::mt19937_64 rand_gen_;
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_CUCKOO_FILTER_SINGLE_TABLE_H_
