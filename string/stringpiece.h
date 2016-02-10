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

// A string-like object that points to a sized piece of memory.

#ifndef CPP_BASE_STRING_STRINGPIECE_H_
#define CPP_BASE_STRING_STRINGPIECE_H_

// Do not add headers here. The only reason for existence of this file is to
// keep the transitive impact of headers small.
#include <stddef.h>
#include <string.h>
#include <string>

#include "cpp-base/type_traits.h"

namespace cpp_base {

class StringPiece {
 private:
  const char*   ptr_;
  int           length_;

 public:
  // We provide non-explicit singleton constructors so users can pass
  // in a "const char*" or a "string" wherever a "StringPiece" is
  // expected.
  StringPiece() : ptr_(NULL), length_(0) {}
  StringPiece(const char* str)  // NOLINT(runtime/explicit)
      : ptr_(str),
        length_((str == NULL) ? 0 : static_cast<int>(strlen(str))) {}
  StringPiece(const std::string& str)  // NOLINT(runtime/explicit)
      : ptr_(str.data()), length_(static_cast<int>(str.size())) {}
  StringPiece(const char* offset, int len) : ptr_(offset), length_(len) {}
  // Substring of another StringPiece.
  // pos must be non-negative and <= x.length().
  StringPiece(const StringPiece& x, int pos);
  // Substring of another StringPiece.
  // pos must be non-negative and <= x.length().
  // len must be non-negative and will be pinned to at most x.length() - pos.
  StringPiece(const StringPiece& x, int pos, int len);

  // data() may return a pointer to a buffer with embedded NULs, and the
  // returned buffer may or may not be null terminated.  Therefore it is
  // typically a mistake to pass data() to a routine that expects a NUL
  // terminated string.
  const char* data() const { return ptr_; }
  int size() const { return length_; }
  int length() const { return length_; }
  bool empty() const { return length_ == 0; }

  void clear() {
    ptr_ = NULL;
    length_ = 0;
  }

  void set(const char* data, int len) {
    ptr_ = data;
    length_ = len;
  }

  void set(const char* str) {
    ptr_ = str;
    if (str != NULL)
      length_ = static_cast<int>(strlen(str));
    else
      length_ = 0;
  }
  void set(const void* data, int len) {
    ptr_ = reinterpret_cast<const char*>(data);
    length_ = len;
  }

  char operator[](int i) const { return ptr_[i]; }

  std::string as_string() const {
    return std::string(data(), size());
  }
  // We also define ToString() here, since many other string-like
  // interfaces name the routine that converts to a C++ string
  // "ToString", and it's confusing to have the method that does that
  // for a StringPiece be called "as_string()".  We also leave the
  // "as_string()" method defined here for existing code.
  std::string ToString() const {
    return std::string(data(), size());
  }

  // standard STL container boilerplate
  typedef char value_type;
  typedef const char* pointer;
  typedef const char& reference;
  typedef const char& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  static const size_type npos;
  typedef const char* const_iterator;
  typedef const char* iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  iterator begin() const { return ptr_; }
  iterator end() const { return ptr_ + length_; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(ptr_ + length_);
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(ptr_);
  }
  // STLS says return size_type, but Google says return int
  int max_size() const { return length_; }
  int capacity() const { return length_; }

  // cpplint.py emits a false positive [build/include_what_you_use]
  int copy(char* buf, size_type n, size_type pos = 0) const;  // NOLINT

  int find(const StringPiece& s, size_type pos = 0) const;
  int find(char c, size_type pos = 0) const;
  int rfind(const StringPiece& s, size_type pos = npos) const;
  int rfind(char c, size_type pos = npos) const;

  int find_first_of(const StringPiece& s, size_type pos = 0) const;
  int find_first_of(char c, size_type pos = 0) const { return find(c, pos); }
  int find_first_not_of(const StringPiece& s, size_type pos = 0) const;
  int find_first_not_of(char c, size_type pos = 0) const;
  int find_last_of(const StringPiece& s, size_type pos = npos) const;
  int find_last_of(char c, size_type pos = npos) const { return rfind(c, pos); }
  int find_last_not_of(const StringPiece& s, size_type pos = npos) const;
  int find_last_not_of(char c, size_type pos = npos) const;

  StringPiece substr(size_type pos, size_type n = npos) const;
};

// This large function is defined inline so that in a fairly common case where
// one of the arguments is a literal, the compiler can elide a lot of the
// following comparisons.
inline bool operator==(const StringPiece& x, const StringPiece& y) {
  int len = x.size();
  if (len != y.size()) {
    return false;
  }

  const char* p1 = x.data();
  const char* p2 = y.data();
  if (p1 == p2) {
    return true;
  }
  if (len <= 0) {
    return true;
  }

  // Test last byte in case strings share large common prefix
  if (p1[len-1] != p2[len-1]) return false;
  if (len == 1) return true;

  // At this point we can, but don't have to, ignore the last byte.  We use
  // this observation to fold the odd-length case into the even-length case.
  len &= ~1;

  return memcmp(p1, p2, len) == 0;
}

inline bool operator!=(const StringPiece& x, const StringPiece& y) {
  return !(x == y);
}

inline bool operator<(const StringPiece& x, const StringPiece& y) {
  const int min_size = x.size() < y.size() ? x.size() : y.size();
  const int r = memcmp(x.data(), y.data(), min_size);
  return (r < 0) || (r == 0 && x.size() < y.size());
}

inline bool operator>(const StringPiece& x, const StringPiece& y) {
  return y < x;
}

inline bool operator<=(const StringPiece& x, const StringPiece& y) {
  return !(x > y);
}

inline bool operator>=(const StringPiece& x, const StringPiece& y) {
  return !(x < y);
}

}  // namespace cpp_base

#endif  // CPP_BASE_STRING_STRINGPIECE_H_
