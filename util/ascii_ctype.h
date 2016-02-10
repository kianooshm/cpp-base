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

#ifndef CPP_BASE_UTIL_ASCII_CTYPE_H_
#define CPP_BASE_UTIL_ASCII_CTYPE_H_

namespace cpp_base {

// ----------------------------------------------------------------------
// ascii_isalpha()
// ascii_isdigit()
// ascii_isalnum()
// ascii_isspace()
// ascii_ispunct()
// ascii_isblank()
// ascii_iscntrl()
// ascii_isxdigit()
// ascii_isprint()
// ascii_isgraph()
// ascii_isupper()
// ascii_islower()
// ascii_tolower()
// ascii_toupper()
//     The ctype.h versions of these routines are slow with some
//     compilers and/or architectures, perhaps because of locale
//     issues.  These versions work for ascii only: they return
//     false for everything above \x7f (which means they return
//     false for any byte from any non-ascii UTF8 character).
//
// The individual bits do not have names because the array definition
// is already tightly coupled to this, and names would make it harder
// to read and debug.
// ----------------------------------------------------------------------

#define kApb kAsciiPropertyBits
extern const unsigned char kAsciiPropertyBits[256];
static inline bool ascii_isalpha(unsigned char c) { return kApb[c] & 0x01; }
static inline bool ascii_isalnum(unsigned char c) { return kApb[c] & 0x04; }
static inline bool ascii_isspace(unsigned char c) { return kApb[c] & 0x08; }
static inline bool ascii_ispunct(unsigned char c) { return kApb[c] & 0x10; }
static inline bool ascii_isblank(unsigned char c) { return kApb[c] & 0x20; }
static inline bool ascii_iscntrl(unsigned char c) { return kApb[c] & 0x40; }
static inline bool ascii_isxdigit(unsigned char c) { return kApb[c] & 0x80; }

static inline bool ascii_isdigit(unsigned char c) {
  return c >= '0' && c <= '9';
}

static inline bool ascii_isprint(unsigned char c) {
  return c >= 32 && c < 127;
}

static inline bool ascii_isgraph(unsigned char c) {
  return c >  32 && c < 127;
}

static inline bool ascii_isupper(unsigned char c) {
  return c >= 'A' && c <= 'Z';
}

static inline bool ascii_islower(unsigned char c) {
  return c >= 'a' && c <= 'z';
}

static inline bool ascii_isascii(unsigned char c) {
  return c < 128;
}
#undef kApb

extern const char kAsciiToLower[256];
static inline char ascii_tolower(unsigned char c) { return kAsciiToLower[c]; }
extern const char kAsciiToUpper[256];
static inline char ascii_toupper(unsigned char c) { return kAsciiToUpper[c]; }

}  // namespace cpp_base

#endif  // CPP_BASE_UTIL_ASCII_CTYPE_H_
