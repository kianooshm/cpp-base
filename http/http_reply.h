/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef CPP_BASE_HTTP_HTTP_REPLY_H_
#define CPP_BASE_HTTP_HTTP_REPLY_H_

#include <string>
#include "cpp-base/callback.h"
#include "cpp-base/http/http_message.h"
#include "cpp-base/http/socket.h"

namespace cpp_base {

class Socket;

// A reply to be sent to a client.
class HttpReply : public HttpMessage {
 public:
  HttpReply() : status_(INTERNAL_SERVER_ERROR), complete_callback_(nullptr) {}
  HttpReply(const HttpReply &copy) : HttpMessage(copy), status_(copy.status_),
                                     complete_callback_(nullptr) {}

  virtual ~HttpReply() {
    if (complete_callback_)
      complete_callback_->Run();
  }

  enum StatusType {
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    MOVED_TEMPORARILY = 302,
    NOT_MODIFIED = 304,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503
  };

  std::string StatusToResponse(StatusType status) const;

  // Get a stock reply.
  void StockReply(StatusType status);

  inline bool IsSuccess() const {
    if (status_ < BAD_REQUEST)
      return true;
    return false;
  }

  inline StatusType status() const {
    return status_;
  }

  inline bool success() {
    return static_cast<uint16_t>(status_) >= 200 &&
           static_cast<uint16_t>(status_) < 400;
  }

  inline bool failure() {
    return static_cast<uint16_t>(status_) >= 400;
  }

  inline void SetStatus(StatusType status) {
    status_ = status;
  }

  // Does not take ownership of closure.
  void AddCompleteCallback(Closure* closure) {
    complete_callback_ = closure;
  }

 protected:
  // The status of the reply.
  StatusType status_;
  Closure* complete_callback_;

  void WriteFirstline(Socket *sock);

  friend class HttpClient;
};

}  // namespace cpp_base

#endif  // CPP_BASE_HTTP_HTTP_REPLY_H_
