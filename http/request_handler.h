/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef CPP_BASE_HTTP_REQUEST_HANDLER_H_
#define CPP_BASE_HTTP_REQUEST_HANDLER_H_

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include "cpp-base/http/http_reply.h"
#include "cpp-base/http/http_request.h"
#include "cpp-base/string/split.h"

namespace cpp_base {

// The common handler for all incoming requests.
class RequestHandler {
  public:
    // Construct with a directory containing files to be served.
    RequestHandler() {}
    ~RequestHandler() {}

    // Add a handler callback for a specific path.
    // This takes a string to match against incoming request paths. If this
    // string ends in "$", a full string match will be done, otherwise any path
    // that begins in this string will match.
    // The 2nd argument is a the callback function that will handle the request.
    // The callback should return true if the request was successfully handled.
    // If it returns false or throws an exception, a 500 error will be returned
    // to the user.
    void AddPath(const std::string& path,
                 std::function<bool(const HttpRequest &, HttpReply *)> handler);

    // Handle a request by forwarding it on to an appropriate callback.
    void HandleRequest(const HttpRequest &req, HttpReply *reply);

  private:
    struct Handler {
        std::string path;
        std::function<bool(const HttpRequest &, HttpReply *)> callback;
    };
    std::vector<Handler> handlers_;
};

}  // namespace cpp_base

#endif  // CPP_BASE_HTTP_REQUEST_HANDLER_H_
