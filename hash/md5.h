/* MD5
 converted to C++ class by Frank Thilo (thilo@unix-ag.org)
 for bzflag (http://www.bzflag.org)

   based on:

   md5.h and md5.c
   reference implementation of RFC 1321

   Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.

*/

#ifndef CPP_BASE_HASH_MD5_H_
#define CPP_BASE_HASH_MD5_H_

#include <glog/logging.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include "cpp-base/integral_types.h"

namespace cpp_base {

// a small class for calculating MD5 hashes of strings or byte arrays
// it is not meant to be fast or secure
//
// usage: 1) feed it blocks of uchars with update()
//        2) finalize()
//        3) get hexdigest() string
//        or
//        MD5(std::string).hexdigest()
//
// assumes that char is 8 bit and int is 32 bit
class MD5 {
 public:
  MD5();
  explicit MD5(const std::string& text);

  // Requires: Finalize() must *not* have been called.
  void Update(const unsigned char *buf, uint32 length);
  void Update(const char *buf, uint32 length);

  MD5& Finalize();

  // Requires: Finalize() must have been called.
  std::string AsHexDigest() const;
  std::string AsRawByteArray() const;

  friend std::ostream& operator<<(std::ostream&, MD5 md5);

 private:
  void Init();

  static const int kBlockSize = 64;

  void Transform(const uint8 block[kBlockSize]);
  static void Decode(uint32 output[], const uint8 input[], uint32 len);
  static void Encode(uint8 output[], const uint32 input[], uint32 len);

  bool finalized;
  uint8 buffer[kBlockSize];  // bytes that didn't fit in last 64 byte chunk
  uint32 count[2];           // 64bit counter for number of bits (lo, hi)
  uint32 state[4];           // digest so far
  uint8 digest[16];          // the result

  // low level logic operations
  static inline uint32 F(uint32 x, uint32 y, uint32 z);
  static inline uint32 G(uint32 x, uint32 y, uint32 z);
  static inline uint32 H(uint32 x, uint32 y, uint32 z);
  static inline uint32 I(uint32 x, uint32 y, uint32 z);
  static inline uint32 rotate_left(uint32 x, int n);
  static inline void FF(uint32 &a, uint32 b, uint32 c,             // NOLINT
                        uint32 d, uint32 x, uint32 s, uint32 ac);
  static inline void GG(uint32 &a, uint32 b, uint32 c,             // NOLINT
                        uint32 d, uint32 x, uint32 s, uint32 ac);
  static inline void HH(uint32 &a, uint32 b, uint32 c,             // NOLINT
                        uint32 d, uint32 x, uint32 s, uint32 ac);
  static inline void II(uint32 &a, uint32 b, uint32 c,             // NOLINT
                        uint32 d, uint32 x, uint32 s, uint32 ac);
};

// When used as a map key, an MD5 value (128 bits) can be represented by two
// uint64s for faster comparisons.
typedef std::pair<uint64, uint64> MD5MapKey;

inline MD5MapKey MakeMD5MapKey(const char* raw_bytes, int len) {
  CHECK_EQ(len, 16);
  // We don't really care about endian-ness here.
  const uint64* ptr = reinterpret_cast<const uint64*>(raw_bytes);
  return std::pair<uint64, uint64>(ptr[0], ptr[1]);
}

inline MD5MapKey MakeMD5MapKey(const std::string& raw_bytes) {
  CHECK_EQ(raw_bytes.size(), 16);
  // We don't really care about endian-ness here.
  const uint64* ptr = reinterpret_cast<const uint64*>(raw_bytes.c_str());
  return std::pair<uint64, uint64>(ptr[0], ptr[1]);
}

}  // namespace cpp_base

#endif  // CPP_BASE_HASH_MD5_H_
