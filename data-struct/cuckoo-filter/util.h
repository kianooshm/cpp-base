// Based on https://github.com/efficient/cuckoofilter by Efficient Computing at Carnegie Mellon

#ifndef CPP_BASE_DATA_STRUCT_CUCKOO_FILTER_HASHUTIL_H_
#define CPP_BASE_DATA_STRUCT_CUCKOO_FILTER_HASHUTIL_H_

#include <sys/types.h>
#include <string>
#include <stdlib.h>
#include <stdint.h>

#include "cpp-base/integral_types.h"

namespace cpp_base {

class HashUtil {
  public:
    // Bob Jenkins Hash
    static uint32_t BobHash(const void *buf, size_t length, uint32_t seed = 0);
    static uint32_t BobHash(const std::string &s, uint32_t seed = 0);

    // Bob Jenkins Hash that returns two indices in one call
    // Useful for Cuckoo hashing, power of two choices, etc.
    // Use idx1 before idx2, when possible. idx1 and idx2 should be initialized to seeds.
    static void BobHash(const void *buf, size_t length, uint32_t *idx1,  uint32_t *idx2);
    static void BobHash(const std::string &s, uint32_t *idx1,  uint32_t *idx2);

    // MurmurHash2
    static uint32_t MurmurHash(const void *buf, size_t length, uint32_t seed = 0);
    static uint32_t MurmurHash(const std::string &s, uint32_t seed = 0);

    // SuperFastHash
    static uint32_t SuperFastHash(const void *buf, size_t len);
    static uint32_t SuperFastHash(const std::string &s);

    // Null hash (shift and mask)
    static uint32_t NullHash(const void* buf, size_t length, uint32_t shiftbytes);

  private:
    HashUtil();
};

// Bit ops utilities.

#define haszero4(x) (((x) - 0x1111ULL) & (~(x)) & 0x8888ULL)
#define hasvalue4(x,n) (haszero4((x) ^ (0x1111ULL * (n))))

#define haszero8(x) (((x) - 0x01010101ULL) & (~(x)) & 0x80808080ULL)
#define hasvalue8(x,n) (haszero8((x) ^ (0x01010101ULL * (n))))

#define haszero12(x) (((x) - 0x001001001001ULL) & (~(x)) & 0x800800800800ULL)
#define hasvalue12(x,n) (haszero12((x) ^ (0x001001001001ULL * (n))))

#define haszero16(x) (((x) - 0x0001000100010001ULL) & (~(x)) & 0x8000800080008000ULL)
#define hasvalue16(x,n) (haszero16((x) ^ (0x0001000100010001ULL * (n))))

inline uint64_t upperpower2(uint64_t x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    x++;
    return x;
}

// Print utilities.

inline std::string bytes_to_hex(const char* data, size_t len) {
    std::string hexstr = "";
    static const char hexes[] = "0123456789ABCDEF ";

    for (size_t i = 0; i < len; i++) {
        unsigned char c = data[i];
        hexstr.push_back(hexes[c >> 4]);
        hexstr.push_back(hexes[c & 0xf]);
        hexstr.push_back(hexes[16]);
    }
    return hexstr;
}


inline std::string bytes_to_hex(const std::string& s) {
    return bytes_to_hex((const char *)s.data(), s.size());
}

inline bool is_power_of_2(int64 x) { return (x & (x - 1)) == 0 || x == 1; }

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_CUCKOO_FILTER_HASHUTIL_H_
