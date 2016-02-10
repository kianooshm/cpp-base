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

#ifndef CPP_BASE_STRING_NUMBERS_H_
#define CPP_BASE_STRING_NUMBERS_H_

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <functional>
#include <limits>
#include <string>
#include <vector>

// A buffer size which is large enough for all the FastToBuffer functions, as
// well as DoubleToBuffer and FloatToBuffer.  We define this here in case other
// string headers depend on it.
static const int kFastToBufferSize = 32;

#include "cpp-base/integral_types.h"
#include "cpp-base/macros.h"
#include "cpp-base/port.h"  // for MUST_USE_RESULT

namespace cpp_base {

// ----------------------------------------------------------------------
// FloatToString()
// IntToString()
// Int64ToString()
// UInt64ToString()
//    Convert various types to their string representation, possibly padded
//    with spaces, using snprintf format specifiers.
//    "Fp" here stands for fingerprint: a 64-bit entity
//    represented in 16 hex digits.
// ----------------------------------------------------------------------

std::string FloatToString(float f, const char* format);
std::string IntToString(int i, const char* format);
std::string Int64ToString(int64 i64, const char* format);
std::string UInt64ToString(uint64 ui64, const char* format);

// The default formats are %7f, %7d, %7lld, and %7llu respectively.
std::string FloatToString(float f);
std::string IntToString(int i);
std::string Int64ToString(int64 i64);
std::string UInt64ToString(uint64 ui64);

// Convert strings to numeric values, with strict error checking.
// Leading and trailing spaces are allowed.
// Negative inputs are not allowed for unsigned ints (unlike strtoul).
// Returns false on errors (including overflow/underflow).
bool safe_strto32(const char* str, int32* value);
bool safe_strto64(const char* str, int64* value);
bool safe_strtou32(const char* str, uint32* value);
bool safe_strtou64(const char* str, uint64* value);
// Convert strings to floating point values.
// Leading and trailing spaces are allowed.
// Values may be rounded on over- and underflow.
bool safe_strtof(const char* str, float* value);
bool safe_strtod(const char* str, double* value);

bool safe_strto32(const std::string& str, int32* value);
bool safe_strto64(const std::string& str, int64* value);
bool safe_strtou32(const std::string& str, uint32* value);
bool safe_strtou64(const std::string& str, uint64* value);
bool safe_strtof(const std::string& str, float* value);
bool safe_strtod(const std::string& str, double* value);

// Parses buffer_size many characters from startptr into value.
bool safe_strto32(const char* startptr, int buffer_size, int32* value);
bool safe_strto64(const char* startptr, int buffer_size, int64* value);

bool safe_strto32_base(const char* str, int32* value, int base);
bool safe_strto64_base(const char* str, int64* value, int base);
bool safe_strtou32_base(const char* str, uint32* value, int base);
bool safe_strtou64_base(const char* str, uint64* value, int base);

bool safe_strto32_base(const std::string& str, int32* value, int base);
bool safe_strto64_base(const std::string& str, int64* value, int base);
bool safe_strtou32_base(const std::string& str, uint32* value, int base);
bool safe_strtou64_base(const std::string& str, uint64* value, int base);

bool safe_strto32_base(const char* startptr, int buffer_size,
                       int32* value, int base);
bool safe_strto64_base(const char* startptr, int buffer_size,
                       int64* value, int base);

// ----------------------------------------------------------------------
// FastIntToBuffer()
// FastHexToBuffer()
// FastHex64ToBuffer()
// FastHex32ToBuffer()
// FastTimeToBuffer()
//    These are intended for speed.  FastIntToBuffer() assumes the
//    integer is non-negative.  FastHexToBuffer() puts output in
//    hex rather than decimal.  FastTimeToBuffer() puts the output
//    into RFC822 format.
//
//    FastHex64ToBuffer() puts a 64-bit unsigned value in hex-format,
//    padded to exactly 16 bytes (plus one byte for '\0')
//
//    FastHex32ToBuffer() puts a 32-bit unsigned value in hex-format,
//    padded to exactly 8 bytes (plus one byte for '\0')
//
//    All functions take the output buffer as an arg.  FastInt() uses
//    at most 22 bytes, FastTime() uses exactly 30 bytes.  They all
//    return a pointer to the beginning of the output, which for
//    FastHex() may not be the beginning of the input buffer.  (For
//    all others, we guarantee that it is.)
//
//    NOTE: In 64-bit land, sizeof(time_t) is 8, so it is possible
//    to pass to FastTimeToBuffer() a time whose year cannot be
//    represented in 4 digits. In this case, the output buffer
//    will contain the string "Invalid:<value>"
// ----------------------------------------------------------------------

// Previously documented minimums -- the buffers provided must be at least this
// long, though these numbers are subject to change:
//     Int32, UInt32:        12 bytes
//     Int64, UInt64, Hex:   22 bytes
//     Time:                 30 bytes
//     Hex32:                 9 bytes
//     Hex64:                17 bytes
// Use kFastToBufferSize rather than hardcoding constants.

char* FastInt32ToBuffer(int32 i, char* buffer);
char* FastInt64ToBuffer(int64 i, char* buffer);
char* FastUInt32ToBuffer(uint32 i, char* buffer);
char* FastUInt64ToBuffer(uint64 i, char* buffer);
char* FastHexToBuffer(int i, char* buffer) MUST_USE_RESULT;
char* FastTimeToBuffer(time_t t, char* buffer);
char* FastHex64ToBuffer(uint64 i, char* buffer);
char* FastHex32ToBuffer(uint32 i, char* buffer);

// at least 22 bytes long
inline char* FastIntToBuffer(int i, char* buffer) {
  return (sizeof(i) == 4 ?
          FastInt32ToBuffer(i, buffer) : FastInt64ToBuffer(i, buffer));
}
inline char* FastUIntToBuffer(unsigned int i, char* buffer) {
  return (sizeof(i) == 4 ?
          FastUInt32ToBuffer(i, buffer) : FastUInt64ToBuffer(i, buffer));
}
inline char* FastLongToBuffer(long i, char* buffer) {               // NOLINT
  return (sizeof(i) == 4 ?
          FastInt32ToBuffer(i, buffer) : FastInt64ToBuffer(i, buffer));
}
inline char* FastULongToBuffer(unsigned long i, char* buffer) {     // NOLINT
  return (sizeof(i) == 4 ?
          FastUInt32ToBuffer(i, buffer) : FastUInt64ToBuffer(i, buffer));
}

// A generic "number type" to buffer template and specializations.
//
// The specialization of FastNumToBuffer<>() should always be made explicit:
//    FastNumToBuffer<int32>(mynums);  // yes
//    FastNumToBuffer(mynums);         // no
template<typename T> char* FastNumToBuffer(T, char*);
template<> inline char* FastNumToBuffer<int32>(int32 i, char* buffer) {
  return FastInt32ToBuffer(i, buffer);
}
template<> inline char* FastNumToBuffer<int64>(int64 i, char* buffer) {
  return FastInt64ToBuffer(i, buffer);
}
template<> inline char* FastNumToBuffer<uint32>(uint32 i, char* buffer) {
  return FastUInt32ToBuffer(i, buffer);
}
template<> inline char* FastNumToBuffer<uint64>(uint64 i, char* buffer) {
  return FastUInt64ToBuffer(i, buffer);
}

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

char* FastInt32ToBufferLeft(int32 i, char* buffer);    // at least 12 bytes
char* FastUInt32ToBufferLeft(uint32 i, char* buffer);    // at least 12 bytes
char* FastInt64ToBufferLeft(int64 i, char* buffer);    // at least 22 bytes
char* FastUInt64ToBufferLeft(uint64 i, char* buffer);    // at least 22 bytes

// Just define these in terms of the above.
inline char* FastUInt32ToBuffer(uint32 i, char* buffer) {
  FastUInt32ToBufferLeft(i, buffer);
  return buffer;
}
inline char* FastUInt64ToBuffer(uint64 i, char* buffer) {
  FastUInt64ToBufferLeft(i, buffer);
  return buffer;
}

// ----------------------------------------------------------------------
// SimpleItoa()
//    Description: converts an integer to a string.
//    Faster than printf("%d").
//
//    Return value: string
// ----------------------------------------------------------------------
std::string SimpleItoa(int32 i);
std::string SimpleItoa(uint32 i);
std::string SimpleItoa(int64 i);
std::string SimpleItoa(uint64 i);

// SimpleAtoi converts a string to an integer.
// Uses safe_strto?() for actual parsing, so strict checking is
// applied, which is to say, the string must be a base-10 integer, optionally
// followed or preceded by whitespace, and value has to be in the range of
// the corresponding integer type.
//
// Returns true if parsing was successful.
template <typename int_type>
bool MUST_USE_RESULT SimpleAtoi(const char* s, int_type* out) {
  // Must be of integer type (not pointer type), with more than 16-bitwidth.
  COMPILE_ASSERT(sizeof(*out) == 4 || sizeof(*out) == 8,
                 SimpleAtoiWorksWith32Or64BitInts);
  if (std::numeric_limits<int_type>::is_signed) {  // Signed
    if (sizeof(*out) == 64 / 8) {  // 64-bit
      return safe_strto64(s, reinterpret_cast<int64*>(out));
    } else {  // 32-bit
      return safe_strto32(s, reinterpret_cast<int32*>(out));
    }
  } else {  // Unsigned
    if (sizeof(*out) == 64 / 8) {  // 64-bit
      return safe_strtou64(s, reinterpret_cast<uint64*>(out));
    } else {  // 32-bit
      return safe_strtou32(s, reinterpret_cast<uint32*>(out));
    }
  }
}

template <typename int_type>
bool MUST_USE_RESULT SimpleAtoi(const std::string& s, int_type* out) {
  return SimpleAtoi(s.c_str(), out);
}

// ----------------------------------------------------------------------
// SimpleDtoa()
// SimpleFtoa()
// DoubleToBuffer()
// FloatToBuffer()
//    Description: converts a double or float to a string which, if
//    passed to strtod(), will produce the exact same original double
//    (except in case of NaN; all NaNs are considered the same value).
//    We try to keep the string short but it's not guaranteed to be as
//    short as possible.
//
//    DoubleToBuffer() and FloatToBuffer() write the text to the given
//    buffer and return it.  The buffer must be at least
//    kDoubleToBufferSize bytes for doubles and kFloatToBufferSize
//    bytes for floats.  kFastToBufferSize is also guaranteed to be large
//    enough to hold either.
//
//    Return value: string
// ----------------------------------------------------------------------
std::string SimpleDtoa(double value);
std::string SimpleFtoa(float value);

char* DoubleToBuffer(double i, char* buffer);
char* FloatToBuffer(float i, char* buffer);

// In practice, doubles should never need more than 24 bytes and floats
// should never need more than 14 (including null terminators), but we
// overestimate to be safe.
static const int kDoubleToBufferSize = 32;
static const int kFloatToBufferSize = 24;

// ----------------------------------------------------------------------
// BasicItoa()
//    Same as SimpleItoa(), but templatized on string type for use with
//    std::wstring and the like.
// ----------------------------------------------------------------------
template <typename BasicString>
BasicString BasicItoa(int32 i);
template <typename BasicString>
BasicString BasicItoa(uint32 i);
template <typename BasicString>
BasicString BasicItoa(int64 i);
template <typename BasicString>
BasicString BasicItoa(uint64 i);

// ----------------------------------------------------------------------
// BasicItoa()
//    Description: converts an integer to a string.
//    Faster than printf("%d").
//
//  BasicString should be a specialization of std::basic_string, or
//  functional equivalent.
// ----------------------------------------------------------------------
template <typename BasicString>
inline BasicString BasicItoa(int32 i) {
  char buf[16];  // Longest is -2147483648
  return BasicString(buf, FastInt32ToBufferLeft(i, buf));
}

// We need this overload because otherwise BasicItoa(5U) wouldn't compile.
template <typename BasicString>
inline BasicString BasicItoa(uint32 i) {
  char buf[16];  // Longest is 4294967295
  return BasicString(buf, FastUInt32ToBufferLeft(i, buf));
}

template <typename BasicString>
inline BasicString BasicItoa(int64 i) {
  char buf[32];  // Longest is -9223372036854775808
  return BasicString(buf, FastInt64ToBufferLeft(i, buf));
}

// We need this overload because otherwise BasicItoa(5ULL) wouldn't compile.
template <typename BasicString>
inline BasicString BasicItoa(uint64 i) {
  char buf[32];  // Longest is 18446744073709551615
  return BasicString(buf, FastUInt64ToBufferLeft(i, buf));
}

}  // namespace cpp_base

#endif  // CPP_BASE_STRING_NUMBERS_H_
