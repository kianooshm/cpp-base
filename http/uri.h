/*
 *  -
 *
 * Copyright 2011 David Parrish <david@dparrish.com>
 *
 * vim: sw=2 tw=120
 *
 */

#ifndef CPP_BASE_HTTP_URI_H_
#define CPP_BASE_HTTP_URI_H_

#include <string>
#include <vector>

namespace cpp_base {

class Uri {
 public:
  Uri() : protocol("http"), port(80) {}

  explicit Uri(const std::string &in) : protocol("http"), port(80) {
    ParseUri(in);
  }

  struct RequestParam {
    std::string key;
    std::string value;
  };

  bool ParseUri(const std::string &in);
  std::vector<RequestParam> ParseQueryString(const std::string &in);
  static bool UrlEncode(const std::string &in, std::string *out);
  static std::string UrlEncode(const std::string &in);
  static bool UrlDecode(const std::string &in, std::string *out);
  static std::string UrlDecode(const std::string &in);
  std::string Assemble() const;
  void SetParam(const std::string &key, const std::string &value);
  void AddParam(const std::string &key, const std::string &value);
  const std::string &GetParam(const std::string &key) const;

  inline void set_protocol(const std::string &newval) {
    protocol = newval;
  }

  inline void set_username(const std::string &newval) {
    username = newval;
  }

  inline void set_password(const std::string &newval) {
    password = newval;
  }

  inline void set_port(uint16_t newval) {
    port = newval;
  }

  inline void set_hostname(const std::string &newval) {
    hostname = newval;
  }

  inline void set_path(const std::string &newval) {
    path = newval;
  }

  inline void set_query_string(const std::string &newval) {
    query_string = newval;
  }

  std::string protocol;
  std::string username;
  std::string password;
  std::string hostname;
  uint16_t port;
  std::string path;
  std::string query_string;
  std::vector<RequestParam> params;
};

}  // namespace cpp_base

#endif  // CPP_BASE_HTTP_URI_H_
