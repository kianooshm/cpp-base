package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cord",
    srcs = ["cord.cc",],
    hdrs = ["cord.h",],
    deps = ["//cpp-base",],
)

cc_test(
    name = "cord_test",
    srcs = ["cord_test.cc",],
    deps = [":cord",
            "//cpp-base/gtest",],
    timeout = "short",
)

cc_library(
    name = "http_message",
    srcs = ["http_message.cc",],
    hdrs = ["http_message.h",
            "http_headers.h",],
    deps = [":cord",
            ":socket",
            ":uri",
            "//cpp-base",
            "//cpp-base/string",],
)

cc_library(
    name = "http_reply",
    srcs = ["http_reply.cc",],
    hdrs = ["http_reply.h"],
    deps = [":http_message",
            ":socket",
            "//cpp-base",],
)

cc_library(
    name = "http_request",
    srcs = ["http_request.cc",],
    hdrs = ["http_request.h"],
    deps = [":http_message",
            ":socket",
            "//cpp-base",],
)

cc_library(
    name = "http_server",
    srcs = ["http_server.cc",],
    hdrs = ["http_server.h",],
    deps = [":request_handler",
            ":socket",
            ":uri",
            "//cpp-base",
            "//cpp-base/management",
            "//cpp-base/string",
            "//cpp-base/util:clock",],
)

cc_library(
    name = "request_handler",
    srcs = ["request_handler.cc"],
    hdrs = ["request_handler.h"],
    deps = [":http_request",
            ":http_reply",
            "//cpp-base",
            "//cpp-base/string",],
)

cc_library(
    name = "socket",
    srcs = ["socket.cc"],
    hdrs = ["socket.h"],
    deps = [":cord",
            "//cpp-base",
            "//cpp-base/string",],
)

cc_test(
    name = "socket_test",
    srcs = ["socket_test.cc",],
    deps = [":socket",
            "//cpp-base/gtest",],
    timeout = "short",
)

cc_library(
    name = "uri",
    srcs = ["uri.cc"],
    hdrs = ["uri.h"],
    deps = ["//cpp-base",
            "//cpp-base/string",],
)

cc_test(
    name = "uri_test",
    srcs = ["uri_test.cc",],
    deps = [":uri",
            "//cpp-base/gtest",],
    timeout = "short",
)
