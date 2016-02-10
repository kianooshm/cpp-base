/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef CPP_BASE_HTTP_SOCKET_H_
#define CPP_BASE_HTTP_SOCKET_H_

#include <string>
#include <vector>

#include "cpp-base/http/cord.h"
#include "cpp-base/integral_types.h"
#include "cpp-base/macros.h"
#include "cpp-base/string/stringprintf.h"

namespace cpp_base {

class Socket {
 public:
  class Address {
   public:
    Address() : address(0), port(0) {}
    Address(uint32_t addr, uint16_t port) : address(addr), port(port) {}
    Address(const std::string &addr, uint16_t port)
            : address(AddressFromString(addr)), port(port) {}
    explicit Address(const Address &src)
            : address(src.address), port(src.port) {}
    explicit Address(const std::string &serverport);

    inline std::string ToString() const {
      return StringPrintf("%s:%u", AddressToString(address).c_str(), port);
    }

    inline std::string AddressToString() const {
      return AddressToString(address);
    }

    uint32_t AddressFromString(const std::string &addr);
    static std::string AddressToString(uint32_t addr);

    // WARNING: Address and port are stored in host byte order and must be
    // converted to network byte order before use by most standard library
    // functions.
    uint32_t address;
    uint16_t port;
  };

  Socket() : fd_(0) {}
  explicit Socket(int fd) : fd_(fd) {}

  ~Socket() {
    Close();
  }

  void Listen(const Address &addr);
  Socket* Accept(int timeout = -1) const;  // Returns nullptr on timeout. Throws exception on error.
  void SetNonblocking(bool nonblocking = true);
  static std::vector<Address> Resolve(const char *hostname);
  void AttemptFlush(int timeout = 0);
  uint64_t Read(int timeout = -1);
  void Abort();
  void Connect(const Address &addr);
  void Connect(const std::string &address, uint16_t port);

  void Flush() {
    // Wait forever to flush output
    AttemptFlush(-1);
  }

  inline void Close() {
    Flush();
    Abort();
  }

  inline void Write(const char* data, int size) {
    write_buffer_.CopyFrom(data, size);
    AttemptFlush();
    Read(0);
  }

  inline void Write(const std::string &str) {
    write_buffer_.CopyFrom(str);
    AttemptFlush();
    Read(0);
  }

  inline void Write(const Cord &cord) {
    write_buffer_.CopyFrom(cord);
    AttemptFlush();
    Read(0);
  }

  inline bool PollRead(int timeout) const;
  inline bool PollWrite(int timeout) const;

  inline const Address &local() const {
    return local_;
  }

  inline const Address &remote() const {
    return remote_;
  }

  Cord *read_buffer() {
    return &read_buffer_;
  }

  // Return the current hostname
  static std::string Hostname();

  int FileDescriptor() const {
    return fd_;
  }

 private:
  int fd_;
  Address local_;
  Address remote_;
  Cord write_buffer_;
  Cord read_buffer_;

  bool Poll(int timeout_ms, uint16_t events) const;

  Address *mutable_remote() {
    return &remote_;
  }

  Address *mutable_local() {
    return &local_;
  }

  DISALLOW_COPY_AND_ASSIGN(Socket);
};

}  // namespace cpp_base

#endif  // CPP_BASE_HTTP_SOCKET_H_
