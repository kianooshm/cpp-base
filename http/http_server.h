/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef CPP_BASE_HTTP_HTTP_SERVER_H_
#define CPP_BASE_HTTP_HTTP_SERVER_H_

#include <signal.h>
#include <memory>
#include <string>
#include <thread>   // NOLINT
#include <vector>
#include "cpp-base/http/request_handler.h"
#include "cpp-base/http/socket.h"

namespace cpp_base {

// The top-level class of the HTTP server.
class HttpServer {
 public:
  // Construct the server to listen on the specified TCP address and port.
  HttpServer(const std::string &address, const uint16_t port);

  ~HttpServer();

  // Start listening on the specified host and port. Launches a new thread
  // for accepting connections -- does not block.
  void Listen();

  // Run the event loop.
  void Start();

  // Stop the server.
  void Stop();

 private:
  // Handles a newly received client connection. Takes ownership of client_socket.
  void HandleClient(Socket* client_socket);

  // Handler for /varz requests.
  bool HandleVarz(const HttpRequest& request, HttpReply* reply);

  // Handler for /configz requests.
  bool HandleConfigz(const HttpRequest& request, HttpReply* reply);

  Socket::Address address_;
  Socket listen_socket_;
  std::unique_ptr<std::thread> listen_thread_;

  // The handler for all incoming requests.
  std::unique_ptr<RequestHandler> request_handler_;

  //// ExportedIntegerSet stats_;

  // Set to true to cause all the listening threads to shut down once they
  // complete the in-progress request.
  bool shutdown_;
};

}  // namespace cpp_base

#endif  // CPP_BASE_HTTP_HTTP_SERVER_H_
