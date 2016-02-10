// Copyright 2010-2014 Google
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

#ifndef CPP_BASE_STRING_SPLIT_H_
#define CPP_BASE_STRING_SPLIT_H_

#include <glog/logging.h>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>

#include "cpp-base/integral_types.h"
#include "cpp-base/string/stringpiece.h"

namespace cpp_base {

// Split a std::string using a nul-terminated list of character
// delimiters.  For each component, parse using the provided
// parsing function and if successful, append it to 'result'.
// Return true if and only if all components parse successfully.
// If there are consecutive delimiters, this function skips over
// all of them.  This function will correctly handle parsing
// strings that have embedded \0s.
template <class T>
bool SplitStringAndParse(StringPiece source, const std::string& delim,
                         bool (*parse)(const std::string& str, T* value),
                         std::vector<T>* result);

// We define here a very truncated version of the powerful strings::Split()
// function. As of 2013-04, it can only be used like this:
// const char* separators = ...;
// std::vector<std::string> result = strings::Split(
//    full, strings::delimiter::AnyOf(separators), strings::SkipEmpty());
//
// TODO(user): The current interface has a really bug prone side effect because
// it can also be used without the AnyOf(). If separators contains only one
// character, this is fine, but if it contains more, then the meaning is
// different: Split() should interpret the whole std::string as a delimiter. Fix
// this.
namespace strings {
std::vector<std::string> Split(const std::string& full,
                               const char* delim, int flags);

// StringPiece version. Its advantages is that it avoids creating a lot of
// small strings. Note however that the full std::string must outlive the usage
// of the result.
std::vector<StringPiece> Split(const std::string& full, const char* delim);

namespace delimiter {
inline const char* AnyOf(const char* x) { return x; }
}  // namespace delimiter

inline int SkipEmpty() { return 0xDEADBEEF; }
}  // namespace strings

// ###################### TEMPLATE INSTANTIATIONS BELOW #######################
template <class T>
bool SplitStringAndParse(const std::string& source, const std::string& delim,
                         bool (*parse)(const std::string& str, T* value),
                         std::vector<T>* result) {
  CHECK(nullptr != parse);
  CHECK(nullptr != result);
  CHECK_GT(delim.size(), 0);
  const std::vector<StringPiece> pieces =
      strings::Split(source, strings::delimiter::AnyOf(delim.c_str()),
                     static_cast<int64>(strings::SkipEmpty()));
  T t;
  for (StringPiece piece : pieces) {
    if (!parse(piece.as_string(), &t)) return false;
    result->push_back(t);
  }
  return true;
}

// Some other utility functions of my own:

inline std::string StringTrim(const std::string& str) {
  std::string::size_type pos1 = str.find_first_not_of(' ');
  std::string::size_type pos2 = str.find_last_not_of(' ');
  return str.substr(pos1 == std::string::npos ? 0 : pos1,
                    pos2 == std::string::npos ? str.length() - 1 : pos2 - pos1 + 1);
}

inline bool StartsWith(const std::string& s1, const std::string& s2) {
    if (s2.size() > s1.size())
        return false;
    return (strncmp(s1.c_str(), s2.c_str(), s2.size()) == 0);
}

inline bool EndsWith(const std::string& s1, const std::string& s2) {
    if (s2.size() > s1.size())
        return false;
    int offset = s1.size() - s2.size();
    return (strncmp(s1.c_str() + offset, s2.c_str(), s2.size()) == 0);
}

inline std::string ConsumeFirstWord(std::string *input) {
  size_t pos = input->find(' ');
  if (pos == std::string::npos) {
    // No spaces, return the whole string
    std::string ret = *input;
    input->clear();
    return ret;
  }
  std::string ret;
  try {
    ret = input->substr(0, pos);
    while (pos < input->length() && input->at(pos) == ' ')
        ++pos;
    input->erase(0, pos);
  } catch (std::exception& e) {
    input->clear();
  }
  return ret;
}

}  // namespace cpp_base

#endif  // CPP_BASE_STRING_SPLIT_H_
