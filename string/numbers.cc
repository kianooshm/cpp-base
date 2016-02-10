// Copyright 2011 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This file contains string processing functions related to
// numeric values.

#include "cpp-base/string/numbers.h"

#include <errno.h>
#include <float.h>          // for DBL_DIG and FLT_DIG
#include <glog/logging.h>
#include <math.h>           // for HUGE_VAL
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits>
#include <string>
#include <vector>

#include "cpp-base/scoped_ptr.h"
#include "cpp-base/string/stringprintf.h"
#include "cpp-base/string/strtoint.h"
#include "cpp-base/util/ascii_ctype.h"

using std::numeric_limits;
using std::string;

namespace cpp_base {

// ----------------------------------------------------------------------
// FloatToString()
// IntToString()
//    Convert various types to their string representation, possibly padded
//    with spaces, using snprintf format specifiers.
// ----------------------------------------------------------------------

string FloatToString(float f, const char* format) {
  char buf[80];
  snprintf(buf, sizeof(buf), format, f);
  return string(buf);
}

string IntToString(int i, const char* format) {
  char buf[80];
  snprintf(buf, sizeof(buf), format, i);
  return string(buf);
}

string Int64ToString(int64 i64, const char* format) {
  char buf[80];
  snprintf(buf, sizeof(buf), format, i64);
  return string(buf);
}

string UInt64ToString(uint64 ui64, const char* format) {
  char buf[80];
  snprintf(buf, sizeof(buf), format, ui64);
  return string(buf);
}

// Default arguments
string FloatToString(float f)   { return FloatToString(f, "%7f"); }
string IntToString(int i)       { return IntToString(i, "%7d"); }
string Int64ToString(int64 i64) {
  return Int64ToString(i64, "%7" GG_LL_FORMAT "d");
}
string UInt64ToString(uint64 ui64) {
  return UInt64ToString(ui64, "%7" GG_LL_FORMAT "u");
}

bool safe_strto32_base(const char* str, int32* value, int base) {
  char* endptr;
  errno = 0;  // errno only gets set on errors
  *value = strto32(str, &endptr, base);
  if (endptr != str) {
    while (ascii_isspace(*endptr)) ++endptr;
  }
  return *str != 0 && *endptr == 0 && errno == 0;
}

bool safe_strto32(const char* startptr, const int buffer_size, int32* value) {
  return safe_strto32_base(startptr, buffer_size, value, 10);
}

bool safe_strto64(const char* startptr, const int buffer_size, int64* value) {
  return safe_strto64_base(startptr, buffer_size, value, 10);
}

namespace {

// kInt32MaxLength[i] is the length of kint32max in base i.
// Bases 0 and 1 are not used.
static const uint8 kInt32MaxLength[] = {
  0, 0, 31, 20, 16, 14, 12, 12, 11, 10, 10, 9, 9, 9, 9, 8, 8,
  8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6 };

// kInt32Cutoff[i] corresponds to kint32max / base i.
// Bases 0 and 1 are not used.
static const int32 kInt32Cutoff[] = {
  0, 0, 1073741823, 715827882, 536870911, 429496729, 357913941,
  306783378, 268435455, 238609294, 214748364, 195225786, 178956970,
  165191049, 153391689, 143165576, 134217727, 126322567, 119304647,
  113025455, 107374182, 102261126, 97612893, 93368854, 89478485,
  85899345, 82595524, 79536431, 76695844, 74051160, 71582788, 69273666,
  67108863, 65075262, 63161283, 61356675, 59652323 };

// kInt32Remain[i] is kint32max % base i.
// Bases 0 and 1 are not used.
static const uint8 kInt32Remain[] = {
  0, 0, 1, 1, 3, 2, 1, 1, 7, 1, 7, 1, 7, 10, 1, 7, 15, 8, 1, 2, 7, 1, 1, 5, 7,
  22, 23, 10, 15, 7, 7, 1, 31, 1, 25, 22, 19 };

// kInt64MaxLength[i] is the length of kint64max in base i.
// Bases 0 and 1 are not used.
static const uint8 kInt64MaxLength[] = {
  0, 0, 63, 40, 32, 28, 25, 23, 21, 20, 19, 19, 18, 18, 17, 17, 16, 16, 16,
  15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 13, 13 };

// kInt64Cutoff[i] corresponds to kint64max / base i.
// Bases 0 and 1 are not used.
static const int64 kInt64Cutoff[] = {
  GG_LONGLONG(0), GG_LONGLONG(0),
  GG_LONGLONG(4611686018427387903), GG_LONGLONG(3074457345618258602),
  GG_LONGLONG(2305843009213693951), GG_LONGLONG(1844674407370955161),
  GG_LONGLONG(1537228672809129301), GG_LONGLONG(1317624576693539401),
  GG_LONGLONG(1152921504606846975), GG_LONGLONG(1024819115206086200),
  GG_LONGLONG(922337203685477580), GG_LONGLONG(838488366986797800),
  GG_LONGLONG(768614336404564650), GG_LONGLONG(709490156681136600),
  GG_LONGLONG(658812288346769700), GG_LONGLONG(614891469123651720),
  GG_LONGLONG(576460752303423487), GG_LONGLONG(542551296285575047),
  GG_LONGLONG(512409557603043100), GG_LONGLONG(485440633518672410),
  GG_LONGLONG(461168601842738790), GG_LONGLONG(439208192231179800),
  GG_LONGLONG(419244183493398900), GG_LONGLONG(401016175515425035),
  GG_LONGLONG(384307168202282325), GG_LONGLONG(368934881474191032),
  GG_LONGLONG(354745078340568300), GG_LONGLONG(341606371735362066),
  GG_LONGLONG(329406144173384850), GG_LONGLONG(318047311615681924),
  GG_LONGLONG(307445734561825860), GG_LONGLONG(297528130221121800),
  GG_LONGLONG(288230376151711743), GG_LONGLONG(279496122328932600),
  GG_LONGLONG(271275648142787523), GG_LONGLONG(263524915338707880),
  GG_LONGLONG(256204778801521550) };

// kInt64Remain[i] is kint64max % base i.
// Bases 0 and 1 are not used.
static const uint8 kInt64Remain[] = {
  0, 0, 1, 1, 3, 2, 1, 0, 7, 7, 7, 7, 7, 7, 7, 7, 15, 8, 7, 17, 7, 7, 7, 2,
  7, 7, 7, 25, 7, 11, 7, 7, 31, 7, 25, 7, 7 };

// Represents integer values of digits.
// Uses 36 to indicate an invalid character since we support
// bases up to 36.
static const uint8 kAsciiToInt[256] = {
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,  // 16 36s.
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
  36, 36, 36, 36, 36, 36, 36,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
  36, 36, 36, 36, 36, 36,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
  36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36 };

// max_length: length of maximum IntType value in base.
// cutoff: array of floor(maximum IntType / base), indexed by base.
// remainder: array of maximum IntType % base, indexed by base.
template<typename IntType, bool is_signed>
bool safe_strtoIntType_base_internal(const char* startptr,
                                     const char* endptr,
                                     IntType* v,
                                     int base,
                                     uint8 max_length,
                                     const IntType* cutoff,
                                     const uint8* remainder) {
  if (startptr == endptr) {
    return false;
  }

  // Sign multiplier: 1 => positive, -1 => negative.
  int sign_multiplier = 1;
  // Whitespace, sign, hexadecimal, and base-0 processing.
  if (*startptr <= '0' || *(endptr - 1) < '0' || base == 0) {
    while (ascii_isspace(*startptr) || ascii_isspace(*(endptr - 1))) {
      if (ascii_isspace(*startptr)) {
        ++startptr;
      } else {
        --endptr;
      }
      if (startptr == endptr) {
        return false;
      }
    }
    if (is_signed) {
      if (*startptr == '-') {
        sign_multiplier = -1;
        if (++startptr == endptr) {
          return false;
        }
      } else if (*startptr == '+') {
        if (++startptr == endptr) {
          return false;
        }
      }
    }
    // Using | 0x20 to convert 'x' or 'X' to lower case.
    if (*startptr == '0' && startptr + 1 < endptr &&
        (base == 16 || base == 0) && ((startptr[1] | 0x20) == 'x')) {
      startptr += 2;
      if (startptr == endptr) {
        return false;
      }
      base = 16;
    } else if (base == 0) {
      base = *startptr == '0' ? 8 : 10;
    }
  }

  int digit;
  IntType value = 0;
  // Integers with length less than max_length do not overflow.
  const int length = endptr - startptr;
  if (length < max_length) {
    // Using Duff's device.
    switch (length & 7) {  // This is just a fast way of computing length % 8.
      case 0: while (startptr < endptr) {
        if ((digit = kAsciiToInt[static_cast<int>(*startptr++)]) >= base) {
          return false;
        }
        value = value * base + digit;
        // Fall through.
      case 7:
        if ((digit = kAsciiToInt[static_cast<int>(*startptr++)]) >= base) {
          return false;
        }
        value = value * base + digit;
        // Fall through.
      case 6:
        if ((digit = kAsciiToInt[static_cast<int>(*startptr++)]) >= base) {
          return false;
        }
        value = value * base + digit;
        // Fall through.
      case 5:
        if ((digit = kAsciiToInt[static_cast<int>(*startptr++)]) >= base) {
          return false;
        }
        value = value * base + digit;
        // Fall through.
      case 4:
        if ((digit = kAsciiToInt[static_cast<int>(*startptr++)]) >= base) {
          return false;
        }
        value = value * base + digit;
        // Fall through.
      case 3:
        if ((digit = kAsciiToInt[static_cast<int>(*startptr++)]) >= base) {
          return false;
        }
        value = value * base + digit;
        // Fall through.
      case 2:
        if ((digit = kAsciiToInt[static_cast<int>(*startptr++)]) >= base) {
          return false;
        }
        value = value * base + digit;
        // Fall through.
      case 1:
        if ((digit = kAsciiToInt[static_cast<int>(*startptr++)]) >= base) {
          return false;
        }
        value = value * base + digit;
      }  // End of while.
    }
  } else {
    while (startptr < endptr) {
      if ((digit = kAsciiToInt[static_cast<int>(*startptr++)]) >= base) {
        return false;
      }
      // Possible overflow.
      if (value >= cutoff[base]) {
        if (startptr == endptr &&
            ((value == cutoff[base] && digit <= remainder[base]) ||
            (sign_multiplier < 0 &&
             ((value == cutoff[base] && digit == remainder[base] + 1)
              || (value - cutoff[base] == 1 && digit == 0 &&
                  remainder[base] == base - 1))))) {
          *v = sign_multiplier * (value * base + digit);
          return true;
        }
        return false;
      }
      value = value * base + digit;
    }
  }
  *v = sign_multiplier * value;
  return true;
}

}  // namespace


bool safe_strto32_base(const char* startptr, const int buffer_size,
                       int32* v, int base) {
  return safe_strtoIntType_base_internal<int32, true>(
      startptr, startptr + buffer_size, v, base, kInt32MaxLength[base],
      kInt32Cutoff, kInt32Remain);
}

bool safe_strto64_base(const char* startptr, const int buffer_size,
                       int64* v, int base) {
  return safe_strtoIntType_base_internal<int64, true>(
      startptr, startptr + buffer_size, v, base, kInt64MaxLength[base],
      kInt64Cutoff, kInt64Remain);
}

bool safe_strto64_base(const char* str, int64* value, int base) {
  char* endptr;
  errno = 0;  // errno only gets set on errors
  *value = strto64(str, &endptr, base);
  if (endptr != str) {
    while (ascii_isspace(*endptr)) ++endptr;
  }
  return *str != 0 && *endptr == 0 && errno == 0;
}

bool safe_strtou32_base(const char* str, uint32* value, int base) {
  // strtoul does not give any errors on negative numbers, so we have to
  // search the string for '-' manually.
  while (ascii_isspace(*str)) ++str;
  if (*str == '-') return false;

  char* endptr;
  errno = 0;  // errno only gets set on errors
  *value = strtou32(str, &endptr, base);
  if (endptr != str) {
    while (ascii_isspace(*endptr)) ++endptr;
  }
  return *str != 0 && *endptr == 0 && errno == 0;
}

bool safe_strtou64_base(const char* str, uint64* value, int base) {
  // strtoull does not give any errors on negative numbers, so we have to
  // search the string for '-' manually.
  while (ascii_isspace(*str)) ++str;
  if (*str == '-') return false;

  char* endptr;
  errno = 0;  // errno only gets set on errors
  *value = strtou64(str, &endptr, base);
  if (endptr != str) {
    while (ascii_isspace(*endptr)) ++endptr;
  }
  return *str != 0 && *endptr == 0 && errno == 0;
}

// Generate functions that wrap safe_strtoXXX_base.
#define GEN_SAFE_STRTO(name, type)                           \
bool name##_base(const string& str, type* value, int base) { \
  return name##_base(str.c_str(), value, base);              \
}                                                            \
bool name(const char* str, type* value) {                    \
  return name##_base(str, value, 10);                        \
}                                                            \
bool name(const string& str, type* value) {                  \
  return name##_base(str.c_str(), value, 10);                \
}
GEN_SAFE_STRTO(safe_strto32, int32);
GEN_SAFE_STRTO(safe_strtou32, uint32);
GEN_SAFE_STRTO(safe_strto64, int64);
GEN_SAFE_STRTO(safe_strtou64, uint64);
#undef GEN_SAFE_STRTO

bool safe_strtof(const char* str, float* value) {
  char* endptr;
#ifdef COMPILER_MSVC  // has no strtof()
  *value = strtod(str, &endptr);
#else
  *value = strtof(str, &endptr);
#endif
  if (endptr != str) {
    while (ascii_isspace(*endptr)) ++endptr;
  }
  // Ignore range errors from strtod/strtof.
  // The values it returns on underflow and
  // overflow are the right fallback in a
  // robust setting.
  return *str != 0 && *endptr == 0;
}

bool safe_strtod(const char* str, double* value) {
  char* endptr;
  *value = strtod(str, &endptr);
  if (endptr != str) {
    while (ascii_isspace(*endptr)) ++endptr;
  }
  // Ignore range errors from strtod.  The values it
  // returns on underflow and overflow are the right
  // fallback in a robust setting.
  return *str != 0 && *endptr == 0;
}

bool safe_strtof(const string& str, float* value) {
  return safe_strtof(str.c_str(), value);
}

bool safe_strtod(const string& str, double* value) {
  return safe_strtod(str.c_str(), value);
}

// ----------------------------------------------------------------------
// FastIntToBuffer()
// FastInt64ToBuffer()
// FastHexToBuffer()
// FastHex64ToBuffer()
// FastHex32ToBuffer()
// FastTimeToBuffer()
//    These are intended for speed.  FastHexToBuffer() assumes the
//    integer is non-negative.  FastHexToBuffer() puts output in
//    hex rather than decimal.  FastTimeToBuffer() puts the output
//    into RFC822 format.  If time is 0, uses the current time.
//
//    FastHex64ToBuffer() puts a 64-bit unsigned value in hex-format,
//    padded to exactly 16 bytes (plus one byte for '\0')
//
//    FastHex32ToBuffer() puts a 32-bit unsigned value in hex-format,
//    padded to exactly 8 bytes (plus one byte for '\0')
//
//       All functions take the output buffer as an arg.  FastInt()
//    uses at most 22 bytes, FastTime() uses exactly 30 bytes.
//    They all return a pointer to the beginning of the output,
//    which may not be the beginning of the input buffer.  (Though
//    for FastTimeToBuffer(), we guarantee that it is.)
// ----------------------------------------------------------------------

char *FastInt64ToBuffer(int64 i, char* buffer) {
  FastInt64ToBufferLeft(i, buffer);
  return buffer;
}

// Offset into buffer where FastInt32ToBuffer places the end of string
// null character.  Also used by FastInt32ToBufferLeft
static const int kFastInt32ToBufferOffset = 11;

char *FastInt32ToBuffer(int32 i, char* buffer) {
  FastInt32ToBufferLeft(i, buffer);
  return buffer;
}

char *FastHexToBuffer(int i, char* buffer) {
  CHECK_GE(i, 0) << "FastHexToBuffer() wants non-negative integers, not " << i;

  static const char *hexdigits = "0123456789abcdef";
  char *p = buffer + 21;
  *p-- = '\0';
  do {
    *p-- = hexdigits[i & 15];   // mod by 16
    i >>= 4;                    // divide by 16
  } while (i > 0);
  return p + 1;
}

char *InternalFastHexToBuffer(uint64 value, char* buffer, int num_byte) {
  static const char *hexdigits = "0123456789abcdef";
  buffer[num_byte] = '\0';
  for (int i = num_byte - 1; i >= 0; i--) {
    buffer[i] = hexdigits[static_cast<uint32>(value) & 0xf];
    value >>= 4;
  }
  return buffer;
}

char *FastHex64ToBuffer(uint64 value, char* buffer) {
  return InternalFastHexToBuffer(value, buffer, 16);
}

char *FastHex32ToBuffer(uint32 value, char* buffer) {
  return InternalFastHexToBuffer(value, buffer, 8);
}

const char two_ASCII_digits[100][2] = {
  {'0', '0'}, {'0', '1'}, {'0', '2'}, {'0', '3'}, {'0', '4'},
  {'0', '5'}, {'0', '6'}, {'0', '7'}, {'0', '8'}, {'0', '9'},
  {'1', '0'}, {'1', '1'}, {'1', '2'}, {'1', '3'}, {'1', '4'},
  {'1', '5'}, {'1', '6'}, {'1', '7'}, {'1', '8'}, {'1', '9'},
  {'2', '0'}, {'2', '1'}, {'2', '2'}, {'2', '3'}, {'2', '4'},
  {'2', '5'}, {'2', '6'}, {'2', '7'}, {'2', '8'}, {'2', '9'},
  {'3', '0'}, {'3', '1'}, {'3', '2'}, {'3', '3'}, {'3', '4'},
  {'3', '5'}, {'3', '6'}, {'3', '7'}, {'3', '8'}, {'3', '9'},
  {'4', '0'}, {'4', '1'}, {'4', '2'}, {'4', '3'}, {'4', '4'},
  {'4', '5'}, {'4', '6'}, {'4', '7'}, {'4', '8'}, {'4', '9'},
  {'5', '0'}, {'5', '1'}, {'5', '2'}, {'5', '3'}, {'5', '4'},
  {'5', '5'}, {'5', '6'}, {'5', '7'}, {'5', '8'}, {'5', '9'},
  {'6', '0'}, {'6', '1'}, {'6', '2'}, {'6', '3'}, {'6', '4'},
  {'6', '5'}, {'6', '6'}, {'6', '7'}, {'6', '8'}, {'6', '9'},
  {'7', '0'}, {'7', '1'}, {'7', '2'}, {'7', '3'}, {'7', '4'},
  {'7', '5'}, {'7', '6'}, {'7', '7'}, {'7', '8'}, {'7', '9'},
  {'8', '0'}, {'8', '1'}, {'8', '2'}, {'8', '3'}, {'8', '4'},
  {'8', '5'}, {'8', '6'}, {'8', '7'}, {'8', '8'}, {'8', '9'},
  {'9', '0'}, {'9', '1'}, {'9', '2'}, {'9', '3'}, {'9', '4'},
  {'9', '5'}, {'9', '6'}, {'9', '7'}, {'9', '8'}, {'9', '9'}
};

// ----------------------------------------------------------------------
// FastInt32ToBufferLeft()
// FastUInt32ToBufferLeft()
// FastInt64ToBufferLeft()
// FastUInt64ToBufferLeft()
//
// Like the Fast*ToBuffer() functions above, these are intended for speed.
// Unlike the Fast*ToBuffer() functions, however, these functions write
// their output to the beginning of the buffer (hence the name, as the
// output is left-aligned).  The caller is responsible for ensuring that
// the buffer has enough space to hold the output.
//
// Returns a pointer to the end of the string (i.e. the null character
// terminating the string).
// ----------------------------------------------------------------------

char* FastUInt32ToBufferLeft(uint32 u, char* buffer) {
  int digits;
  const char *ASCII_digits = NULL;
  // The idea of this implementation is to trim the number of divides to as few
  // as possible by using multiplication and subtraction rather than mod (%),
  // and by outputting two digits at a time rather than one.
  // The huge-number case is first, in the hopes that the compiler will output
  // that case in one branch-free block of code, and only output conditional
  // branches into it from below.
  if (u >= 1000000000) {  // >= 1,000,000,000
    digits = u / 100000000;  // 100,000,000
    ASCII_digits = two_ASCII_digits[digits];
    buffer[0] = ASCII_digits[0];
    buffer[1] = ASCII_digits[1];
    buffer += 2;
 sublt100_000_000:
    u -= digits * 100000000;  // 100,000,000
 lt100_000_000:
    digits = u / 1000000;  // 1,000,000
    ASCII_digits = two_ASCII_digits[digits];
    buffer[0] = ASCII_digits[0];
    buffer[1] = ASCII_digits[1];
    buffer += 2;
 sublt1_000_000:
    u -= digits * 1000000;  // 1,000,000
 lt1_000_000:
    digits = u / 10000;  // 10,000
    ASCII_digits = two_ASCII_digits[digits];
    buffer[0] = ASCII_digits[0];
    buffer[1] = ASCII_digits[1];
    buffer += 2;
 sublt10_000:
    u -= digits * 10000;  // 10,000
 lt10_000:
    digits = u / 100;
    ASCII_digits = two_ASCII_digits[digits];
    buffer[0] = ASCII_digits[0];
    buffer[1] = ASCII_digits[1];
    buffer += 2;
 sublt100:
    u -= digits * 100;
 lt100:
    digits = u;
    ASCII_digits = two_ASCII_digits[digits];
    buffer[0] = ASCII_digits[0];
    buffer[1] = ASCII_digits[1];
    buffer += 2;
 done:
    *buffer = 0;
    return buffer;
  }

  if (u < 100) {
    digits = u;
    if (u >= 10) goto lt100;
    *buffer++ = '0' + digits;
    goto done;
  }
  if (u  <  10000) {   // 10,000
    if (u >= 1000) goto lt10_000;
    digits = u / 100;
    *buffer++ = '0' + digits;
    goto sublt100;
  }
  if (u  <  1000000) {   // 1,000,000
    if (u >= 100000) goto lt1_000_000;
    digits = u / 10000;  //    10,000
    *buffer++ = '0' + digits;
    goto sublt10_000;
  }
  if (u  <  100000000) {   // 100,000,000
    if (u >= 10000000) goto lt100_000_000;
    digits = u / 1000000;  //   1,000,000
    *buffer++ = '0' + digits;
    goto sublt1_000_000;
  }
  // we already know that u < 1,000,000,000
  digits = u / 100000000;   // 100,000,000
  *buffer++ = '0' + digits;
  goto sublt100_000_000;
}

char* FastInt32ToBufferLeft(int32 i, char* buffer) {
  uint32 u = i;
  if (i < 0) {
    *buffer++ = '-';
    u = -i;
  }
  return FastUInt32ToBufferLeft(u, buffer);
}

char* FastUInt64ToBufferLeft(uint64 u64, char* buffer) {
  int digits;
  const char *ASCII_digits = NULL;

  uint32 u = static_cast<uint32>(u64);
  if (u == u64) return FastUInt32ToBufferLeft(u, buffer);

  uint64 top_11_digits = u64 / 1000000000;
  buffer = FastUInt64ToBufferLeft(top_11_digits, buffer);
  u = u64 - (top_11_digits * 1000000000);

  digits = u / 10000000;  // 10,000,000
  DCHECK_LT(digits, 100);
  ASCII_digits = two_ASCII_digits[digits];
  buffer[0] = ASCII_digits[0];
  buffer[1] = ASCII_digits[1];
  buffer += 2;
  u -= digits * 10000000;  // 10,000,000
  digits = u / 100000;  // 100,000
  ASCII_digits = two_ASCII_digits[digits];
  buffer[0] = ASCII_digits[0];
  buffer[1] = ASCII_digits[1];
  buffer += 2;
  u -= digits * 100000;  // 100,000
  digits = u / 1000;  // 1,000
  ASCII_digits = two_ASCII_digits[digits];
  buffer[0] = ASCII_digits[0];
  buffer[1] = ASCII_digits[1];
  buffer += 2;
  u -= digits * 1000;  // 1,000
  digits = u / 10;
  ASCII_digits = two_ASCII_digits[digits];
  buffer[0] = ASCII_digits[0];
  buffer[1] = ASCII_digits[1];
  buffer += 2;
  u -= digits * 10;
  digits = u;
  *buffer++ = '0' + digits;
  *buffer = 0;
  return buffer;
}

char* FastInt64ToBufferLeft(int64 i, char* buffer) {
  uint64 u = i;
  if (i < 0) {
    *buffer++ = '-';
    u = -i;
  }
  return FastUInt64ToBufferLeft(u, buffer);
}

// ----------------------------------------------------------------------
// SimpleItoa()
//    Description: converts an integer to a string.
//    Faster than printf("%d").
//
//    Return value: string
// ----------------------------------------------------------------------
string SimpleItoa(int32 i) {
  return BasicItoa<string>(i);
}

// We need this overload because otherwise SimpleItoa(5U) wouldn't compile.
string SimpleItoa(uint32 i) {
  return BasicItoa<string>(i);
}

string SimpleItoa(int64 i) {
  return BasicItoa<string>(i);
}

// We need this overload because otherwise SimpleItoa(5ULL) wouldn't compile.
string SimpleItoa(uint64 i) {
  return BasicItoa<string>(i);
}

// ----------------------------------------------------------------------
// SimpleDtoa()
// SimpleFtoa()
// DoubleToBuffer()
// FloatToBuffer()
//    We want to print the value without losing precision, but we also do
//    not want to print more digits than necessary.  This turns out to be
//    trickier than it sounds.  Numbers like 0.2 cannot be represented
//    exactly in binary.  If we print 0.2 with a very large precision,
//    e.g. "%.50g", we get "0.2000000000000000111022302462515654042363167".
//    On the other hand, if we set the precision too low, we lose
//    significant digits when printing numbers that actually need them.
//    It turns out there is no precision value that does the right thing
//    for all numbers.
//
//    Our strategy is to first try printing with a precision that is never
//    over-precise, then parse the result with strtod() to see if it
//    matches.  If not, we print again with a precision that will always
//    give a precise result, but may use more digits than necessary.
//
//    An arguably better strategy would be to use the algorithm described
//    in "How to Print Floating-Point Numbers Accurately" by Steele &
//    White, e.g. as implemented by David M. Gay's dtoa().  It turns out,
//    however, that the following implementation is about as fast as
//    DMG's code.  Furthermore, DMG's code locks mutexes, which means it
//    will not scale well on multi-core machines.  DMG's code is slightly
//    more accurate (in that it will never use more digits than
//    necessary), but this is probably irrelevant for most users.
//
//    Rob Pike and Ken Thompson also have an implementation of dtoa() in
//    third_party/fmt/fltfmt.cc.  Their implementation is similar to this
//    one in that it makes guesses and then uses strtod() to check them.
//    Their implementation is faster because they use their own code to
//    generate the digits in the first place rather than use snprintf(),
//    thus avoiding format string parsing overhead.  However, this makes
//    it considerably more complicated than the following implementation,
//    and it is embedded in a larger library.  If speed turns out to be
//    an issue, we could re-implement this in terms of their
//    implementation.
// ----------------------------------------------------------------------

string SimpleDtoa(double value) {
  char buffer[kDoubleToBufferSize];
  return DoubleToBuffer(value, buffer);
}

string SimpleFtoa(float value) {
  char buffer[kFloatToBufferSize];
  return FloatToBuffer(value, buffer);
}

char* DoubleToBuffer(double value, char* buffer) {
  // DBL_DIG is 15 for IEEE-754 doubles, which are used on almost all
  // platforms these days.  Just in case some system exists where DBL_DIG
  // is significantly larger -- and risks overflowing our buffer -- we have
  // this assert.
  COMPILE_ASSERT(DBL_DIG < 20, DBL_DIG_is_too_big);

  int snprintf_result =
    snprintf(buffer, kDoubleToBufferSize, "%.*g", DBL_DIG, value);

  // The snprintf should never overflow because the buffer is significantly
  // larger than the precision we asked for.
  DCHECK(snprintf_result > 0 && snprintf_result < kDoubleToBufferSize);

  if (strtod(buffer, NULL) != value) {
    snprintf_result =
      snprintf(buffer, kDoubleToBufferSize, "%.*g", DBL_DIG+2, value);

    // Should never overflow; see above.
    DCHECK(snprintf_result > 0 && snprintf_result < kDoubleToBufferSize);
  }
  return buffer;
}

char* FloatToBuffer(float value, char* buffer) {
  // FLT_DIG is 6 for IEEE-754 floats, which are used on almost all
  // platforms these days.  Just in case some system exists where FLT_DIG
  // is significantly larger -- and risks overflowing our buffer -- we have
  // this assert.
  COMPILE_ASSERT(FLT_DIG < 10, FLT_DIG_is_too_big);

  int snprintf_result =
    snprintf(buffer, kFloatToBufferSize, "%.*g", FLT_DIG, value);

  // The snprintf should never overflow because the buffer is significantly
  // larger than the precision we asked for.
  DCHECK(snprintf_result > 0 && snprintf_result < kFloatToBufferSize);

  float parsed_value;
  if (!safe_strtof(buffer, &parsed_value) || parsed_value != value) {
    snprintf_result =
      snprintf(buffer, kFloatToBufferSize, "%.*g", FLT_DIG+2, value);

    // Should never overflow; see above.
    DCHECK(snprintf_result > 0 && snprintf_result < kFloatToBufferSize);
  }
  return buffer;
}

}  // namespace cpp_base
