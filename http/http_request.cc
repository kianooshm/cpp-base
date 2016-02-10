/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#include "cpp-base/http/http_request.h"
#include <string>
#include <vector>

using std::string;

namespace cpp_base {

bool HttpRequest::HasParam(const string &key) const {
  for (const auto& param : uri.params) {
    if (param.key == key)
      return true;
  }
  return false;
}

const string &HttpRequest::GetParam(const string &key) const {
  static const string empty_string("");
  for (const auto& param : uri.params) {
    if (param.key == key)
      return param.value;
  }
  return empty_string;
}

void HttpRequest::WriteFirstline(Socket *sock) {
  Cord firstline;
  firstline.CopyFrom(StringPrintf("%s %s %s\r\n", method().c_str(),
                                                  uri.Assemble().c_str(),
                                                  http_version().c_str()));
  sock->Write(firstline);
  sock->Flush();
}

}  // namespace cpp_base
