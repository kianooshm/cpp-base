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

// Functions for joining strings.

#ifndef CPP_BASE_STRING_JOIN_H_
#define CPP_BASE_STRING_JOIN_H_

#include <stdio.h>
#include <string.h>

#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "cpp-base/macros.h"
#include "cpp-base/string/numbers.h"
#include "cpp-base/string/stringpiece.h"
#include "cpp-base/template_util.h"

namespace cpp_base {

// The AlphaNum type is designed for internal use by Join, though
// I suppose that any routine accepting either a string or a number could
// accept it.  The basic idea is that by accepting a "const AlphaNum &" as an
// argument to your function, your callers will automagically convert bools,
// integers, and floating point values to strings for you.
//
// Conversion from 8-bit values is not accepted because if it were, then an
// attempt to pass ':' instead of ":" might result in a 58 ending up in your
// result.
//
// Bools convert to "0" or "1".
//
// Floating point values are converted to a string which, if
// passed to strtod(), would produce the exact same original double
// (except in case of NaN; all NaNs are considered the same value).
// We try to keep the string short but it's not guaranteed to be as
// short as possible.
//
// This class has implicit constructors.

struct AlphaNum {
  StringPiece piece;
  char digits[kFastToBufferSize];

  // No bool ctor -- bools convert to an integral type.
  // A bool ctor would also convert incoming pointers (bletch).

  AlphaNum(int32 i32)  // NOLINT(runtime/explicit)
      : piece(digits, FastInt32ToBufferLeft(i32, digits) - &digits[0]) {}
  AlphaNum(uint32 u32)  // NOLINT(runtime/explicit)
      : piece(digits, FastUInt32ToBufferLeft(u32, digits) - &digits[0]) {}
  AlphaNum(int64 i64)  // NOLINT(runtime/explicit)
      : piece(digits, FastInt64ToBufferLeft(i64, digits) - &digits[0]) {}
  AlphaNum(uint64 u64)  // NOLINT(runtime/explicit)
      : piece(digits, FastUInt64ToBufferLeft(u64, digits) - &digits[0]) {}

  AlphaNum(float f)  // NOLINT(runtime/explicit)
    : piece(digits, strlen(FloatToBuffer(f, digits))) {}
  AlphaNum(double f)  // NOLINT(runtime/explicit)
    : piece(digits, strlen(DoubleToBuffer(f, digits))) {}

  AlphaNum(const char *c_str) : piece(c_str) {}  // NOLINT(runtime/explicit)
  AlphaNum(const StringPiece &pc) : piece(pc) {}  // NOLINT(runtime/explicit)
  AlphaNum(const std::string &s) : piece(s) {}  // NOLINT(runtime/explicit)

  StringPiece::size_type size() const { return piece.size(); }
  const char *data() const { return piece.data(); }

 private:
  // Use ":" not ':'
  AlphaNum(char c);  // NOLINT(runtime/explicit)
};

extern AlphaNum gEmptyAlphaNum;

// ----------------------------------------------------------------------
// StrCat()
//    This merges the given strings or numbers, with no delimiter.  This
//    is designed to be the fastest possible way to construct a string out
//    of a mix of raw C strings, StringPieces, strings, bool values,
//    and numeric values.
//
//    Don't use this for user-visible strings.  The localization process
//    works poorly on strings built up out of fragments.
//
//    For clarity and performance, don't use StrCat when appending to a
//    string.  In particular, avoid using any of these (anti-)patterns:
//      str.append(StrCat(...)
//      str += StrCat(...)
//      str = StrCat(str, ...)
//    where the last is the worse, with the potential to change a loop
//    from a linear time operation with O(1) dynamic allocations into a
//    quadratic time operation with O(n) dynamic allocations.  StrAppend
//    is a better choice than any of the above, subject to the restriction
//    of StrAppend(&str, a, b, c, ...) that none of the a, b, c, ... may
//    be a reference into str.
// ----------------------------------------------------------------------

std::string StrCat(const AlphaNum &a);
std::string StrCat(const AlphaNum &a, const AlphaNum &b);
std::string StrCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c);
std::string StrCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                   const AlphaNum &d);
std::string StrCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                   const AlphaNum &d, const AlphaNum &e);
std::string StrCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                   const AlphaNum &d, const AlphaNum &e, const AlphaNum &f);
std::string StrCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                   const AlphaNum &d, const AlphaNum &e, const AlphaNum &f,
                   const AlphaNum &g);
std::string StrCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                   const AlphaNum &d, const AlphaNum &e, const AlphaNum &f,
                   const AlphaNum &g, const AlphaNum &h);

// Support up to 12 params by using a default empty AlphaNum.
std::string StrCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                   const AlphaNum &d, const AlphaNum &e, const AlphaNum &f,
                   const AlphaNum &g, const AlphaNum &h, const AlphaNum &i,
                   const AlphaNum &j = gEmptyAlphaNum,
                   const AlphaNum &k = gEmptyAlphaNum,
                   const AlphaNum &l = gEmptyAlphaNum);

// ----------------------------------------------------------------------
// StrAppend()
//    Same as above, but adds the output to the given string.
//    WARNING: For speed, StrAppend does not try to check each of its input
//    arguments to be sure that they are not a subset of the string being
//    appended to.  That is, while this will work:
//
//    string s = "foo";
//    s += s;
//
//    This will not (necessarily) work:
//
//    string s = "foo";
//    StrAppend(&s, s);
//
//    Note: while StrCat supports appending up to 12 arguments, StrAppend
//    is currently limited to 9.  That's rarely an issue except when
//    automatically transforming StrCat to StrAppend, and can easily be
//    worked around as consecutive calls to StrAppend are quite efficient.
// ----------------------------------------------------------------------

void StrAppend(std::string *dest, const AlphaNum &a);
void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b);
void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b,
               const AlphaNum &c);
void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b,
               const AlphaNum &c, const AlphaNum &d);

// Support up to 9 params by using a default empty AlphaNum.
void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b,
               const AlphaNum &c, const AlphaNum &d, const AlphaNum &e,
               const AlphaNum &f = gEmptyAlphaNum,
               const AlphaNum &g = gEmptyAlphaNum,
               const AlphaNum &h = gEmptyAlphaNum,
               const AlphaNum &i = gEmptyAlphaNum);

// ----------------------------------------------------------------------
// JoinStrings(), JoinStringsIterator(), JoinStringsInArray()
//
//    JoinStrings concatenates a container of strings into a C++ string,
//    using the string "delim" as a separator between components.
//    "components" can be any sequence container whose values are C++ strings
//    or StringPieces. More precisely, "components" must support STL container
//    iteration; i.e. it must have begin() and end() methods with appropriate
//    semantics, which return forward iterators whose value type is
//    string or StringPiece. Repeated string fields of protocol messages
//    satisfy these requirements.
//
//    JoinStringsIterator is the same as JoinStrings, except that the input
//    strings are specified with a pair of iterators. The requirements on
//    the iterators are the same as the requirements on components.begin()
//    and components.end() for JoinStrings.
//
//    JoinStringsInArray is the same as JoinStrings, but operates on
//    an array of C++ strings or string pointers.
//
//    There are two flavors of each function, one flavor returns the
//    concatenated string, another takes a pointer to the target string. In
//    the latter case the target string is cleared and overwritten.
// ----------------------------------------------------------------------
template <class CONTAINER>
void JoinStrings(const CONTAINER& components,
                 const StringPiece& delim,
                 std::string* result);
template <class CONTAINER>
std::string JoinStrings(const CONTAINER& components,
                        const StringPiece& delim);

template <class ITERATOR>
void JoinStringsIterator(const ITERATOR& start,
                         const ITERATOR& end,
                         const StringPiece& delim,
                         std::string* result);
template <class ITERATOR>
std::string JoinStringsIterator(const ITERATOR& start,
                                const ITERATOR& end,
                                const StringPiece& delim);

template<typename ITERATOR>
void JoinKeysAndValuesIterator(const ITERATOR& start,
                               const ITERATOR& end,
                               const StringPiece& intra_delim,
                               const StringPiece& inter_delim,
                               std::string *result) {
  result->clear();

  // precompute resulting length so we can reserve() memory in one shot.
  if (start != end) {
    int dist = distance(start, end);
    int length = inter_delim.size() * (dist - 1) + intra_delim.size() * dist;
    for (ITERATOR iter = start; iter != end; ++iter) {
      length += iter->first.size() + iter->second.size();
    }
    result->reserve(length);
  }

  // Now combine everything.
  for (ITERATOR iter = start; iter != end; ++iter) {
    if (iter != start) {
      result->append(inter_delim.data(), inter_delim.size());
    }
    result->append(iter->first.data(), iter->first.size());
    result->append(intra_delim.data(), intra_delim.size());
    result->append(iter->second.data(), iter->second.size());
  }
}

void JoinStringsInArray(std::string const* const* components,
                        int num_components,
                        const char* delim,
                        std::string* result);
void JoinStringsInArray(std::string const* components,
                        int num_components,
                        const char* delim,
                        std::string* result);
std::string JoinStringsInArray(std::string const* const* components,
                               int num_components,
                               const char* delim);
std::string JoinStringsInArray(std::string const* components,
                               int num_components,
                               const char* delim);

// ----------------------------------------------------------------------
// Definitions of above JoinStrings* methods
// ----------------------------------------------------------------------
template <class CONTAINER>
inline void JoinStrings(const CONTAINER& components,
                        const StringPiece& delim,
                        std::string* result) {
  JoinStringsIterator(components.begin(), components.end(), delim, result);
}

template <class CONTAINER>
inline std::string JoinStrings(const CONTAINER& components,
                               const StringPiece& delim) {
  std::string result;
  JoinStrings(components, delim, &result);
  return result;
}

template <class ITERATOR>
void JoinStringsIterator(const ITERATOR& start,
                         const ITERATOR& end,
                         const StringPiece& delim,
                         std::string* result) {
  result->clear();

  // Precompute resulting length so we can reserve() memory in one shot.
  if (start != end) {
    int length = delim.size()*(distance(start, end)-1);
    for (ITERATOR iter = start; iter != end; ++iter) {
      length += iter->size();
    }
    result->reserve(length);
  }

  // Now combine everything.
  for (ITERATOR iter = start; iter != end; ++iter) {
    if (iter != start) {
      result->append(delim.data(), delim.size());
    }
    result->append(iter->data(), iter->size());
  }
}

template <class ITERATOR>
inline std::string JoinStringsIterator(const ITERATOR& start,
                                       const ITERATOR& end,
                                       const StringPiece& delim) {
  std::string result;
  JoinStringsIterator(start, end, delim, &result);
  return result;
}

inline std::string JoinStringsInArray(std::string const* const* components,
                                      int num_components,
                                      const char* delim) {
  std::string result;
  JoinStringsInArray(components, num_components, delim, &result);
  return result;
}

inline std::string JoinStringsInArray(std::string const* components,
                                      int num_components,
                                      const char* delim) {
  std::string result;
  JoinStringsInArray(components, num_components, delim, &result);
  return result;
}

}  // namespace cpp_base

#endif  // CPP_BASE_STRING_JOIN_H_
