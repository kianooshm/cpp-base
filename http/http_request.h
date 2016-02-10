/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef CPP_BASE_HTTP_HTTP_REQUEST_H_
#define CPP_BASE_HTTP_HTTP_REQUEST_H_

#include <string>
#include "cpp-base/http/http_message.h"
#include "cpp-base/http/socket.h"

namespace cpp_base {

// A request received from a client.
class HttpRequest : public HttpMessage {
 public:
  HttpRequest() {}
  bool HasParam(const std::string &key) const;
  const std::string &GetParam(const std::string &key) const;

  std::string url;
  Uri uri;
  Socket::Address source;

 private:
  void WriteFirstline(Socket *sock);
};

}  // namespace cpp_base

#endif  // CPP_BASE_HTTP_HTTP_REQUEST_H_
