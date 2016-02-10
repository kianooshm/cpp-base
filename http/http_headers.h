/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef CPP_BASE_HTTP_HTTP_HEADERS_H_
#define CPP_BASE_HTTP_HTTP_HEADERS_H_

#include <string>
#include <vector>
#include "cpp-base/string/join.h"

namespace cpp_base {

class HttpHeader {
 public:
  HttpHeader(const std::string &name, const std::string &value)
    : name(name),
      value(value) {}
  std::string name;
  std::string value;
};

class HttpHeaders {
 public:
  // Sets the value of a header "name" to "value". If the header already exists,
  // its value is replaced.
  void SetHeader(const std::string &name, const std::string &value) {
    for (size_t i = 0; i < headers_.size(); ++i) {
      if (headers_[i].name == name) {
        headers_[i].value = value;
        return;
      }
    }
    AddHeader(name, value);
  }

  void SetHeader(const std::string &name, uint64_t value) {
    SetHeader(name, StrCat(value));
  }

  // Sets the value of a header "name" to "value". If the header already
  // exists, another header will be added with the new value.
  void AddHeader(const std::string &name, const std::string &value) {
    headers_.push_back(HttpHeader(name, value));
  }

  void AddHeader(const std::string &name, uint64_t value) {
    AddHeader(name, StrCat(value));
  }

  void AppendLastHeader(const std::string append) {
    HttpHeader last = headers_.back();
    headers_.pop_back();
    last.value += append;
    headers_.push_back(last);
  }

  // Checks whether the header is set at all. Does not check how many instances
  // of a header are set.
  bool HeaderExists(const std::string &name) const {
    for (size_t i = 0; i < headers_.size(); ++i) {
      if (headers_[i].name == name)
        return true;
    }
    return false;
  }

  void RemoveHeader(const std::string &name) {
    for (std::vector<HttpHeader>::iterator i = headers_.begin();
         i != headers_.end(); ++i) {
      if (i->name == name) {
        headers_.erase(i);
        i = headers_.begin();
        continue;
      }
    }
  }

  // Gets the first value of a header from the set.
  const std::string &GetHeader(const std::string &name) const {
    static std::string emptystring;
    for (size_t i = 0; i < headers_.size(); ++i) {
      if (headers_[i].name == name)
        return headers_[i].value;
    }
    return emptystring;
  }

  // Gets every value of a set header.
  std::vector<std::string> GetHeaderValues(const std::string &name) const {
    std::vector<std::string> values;
    for (size_t i = 0; i < headers_.size(); ++i) {
      if (headers_[i].name == name)
        values.push_back(headers_[i].value);
    }
    return values;
  }

  bool empty() const {
    return headers_.empty();
  }

  size_t size() const {
    return headers_.size();
  }

  const HttpHeader &back() const {
    return headers_.back();
  }

  const HttpHeader &operator[](int index) const {
    return headers_[index];
  }

 private:
  std::vector<HttpHeader> headers_;
};

}  // namespace cpp_base

#endif  // CPP_BASE_HTTP_HTTP_HEADERS_H_
