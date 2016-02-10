/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef CPP_BASE_HTTP_HTTP_MESSAGE_H_
#define CPP_BASE_HTTP_HTTP_MESSAGE_H_

#include <string>
#include "cpp-base/http/cord.h"
#include "cpp-base/http/http_headers.h"
#include "cpp-base/http/uri.h"
#include "cpp-base/string/stringpiece.h"

namespace cpp_base {

class Socket;

class HttpMessage {
 public:
  HttpMessage()
    : method_("GET"),
      version_major_(0),
      version_minor_(0),
      header_written_(false),
      status_written_(false),
      chunked_encoding_(false) {}
  virtual ~HttpMessage() {}

  void WriteHeader(Socket *sock);
  void set_http_version(const std::string &version);
  std::string http_version() const;
  void SetHeader(const char *key, const char *value);
  const HttpHeaders &headers() const;
  void SetContentLength(size_t length = 0);
  uint64_t GetContentLength() const;
  void SetContentType(const char *type);
  const std::string &GetContentType();
  void Write(Socket *sock);
  void ReadAndParseHeaders(Socket *sock, int deadline_ms);

  const Cord &body() const {
    return body_;
  }

  Cord *mutable_body() {
    return &body_;
  }

  const std::string &method() const {
    return method_;
  }

  void set_method(const std::string &method) {
    method_ = method;
  }

  bool chunked_encoding() const {
    return chunked_encoding_;
  }

  void set_chunked_encoding(bool newval = true) {
    chunked_encoding_ = newval;
  }

 protected:
  std::string method_;
  uint8_t version_major_;
  uint8_t version_minor_;
  bool header_written_;
  bool status_written_;
  bool chunked_encoding_;
  HttpHeaders headers_;

  Cord body_;

  // Override this for subclasses. This is the first line that is written.
  virtual void WriteFirstline(Socket *sock) = 0;

  void WriteChunk(Socket *sock, const StringPiece &chunk);
  void WriteLastChunk(Socket *sock);

  inline HttpHeaders *mutable_headers() {
    return &headers_;
  }

  static const char *crlf_;
  static const char *header_sep_;

  friend class HttpServer;
  friend class HttpClient;
};

inline char *HexToBuffer(uint32_t i, char *buffer) {
  static const char *hexdigits = "0123456789abcdef";
  char *p = buffer + 21;
  // Write characters to the buffer starting at the end and work backwards.
  *p-- = '\0';
  do {
    *p-- = hexdigits[i & 15];   // mod by 16
    i >>= 4;                    // divide by 16
  } while (i > 0);
  return p + 1;
}

inline std::string HexToBuffer(uint32_t i) {
  char buf[21];
  char *p = HexToBuffer(i, buf);
  return p;
}

}  // namespace cpp_base

#endif  // CPP_BASE_HTTP_HTTP_MESSAGE_H_
