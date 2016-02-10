package(default_visibility = ["//visibility:public"])

cc_library(
    name = "util",
    srcs = [],
    hdrs = [],
    deps = [":ascii_ctype",
            ":bitset",
            ":clock",
            ":grpc_mock",
            ":map_util",
            ":math_limits",
            ":math_util",
            ":periodic_closure",
            ":random",
            ":stl_util",
            ":sysinfo",
            ":triple",],
)

cc_library(
    name = "ascii_ctype",
    srcs = ["ascii_ctype.cc"],
    hdrs = ["ascii_ctype.h"],
    deps = [],
)

cc_library(
    name = "bitset",
    srcs = ["bitset.cc"],
    hdrs = ["bitset.h"],
    deps = ["//cpp-base",],
)

cc_library(
    name = "clock",
    srcs = ["clock.cc"],
    hdrs = ["clock.h"],
    deps = [":map_util",
            "//cpp-base",
            "//cpp-base/string:join",],
)

cc_library(
    name = "grpc_mock",
    srcs = [],
    hdrs = ["grpc_mock.h"],
    deps = [],
)

cc_library(
    name = "map_util",
    srcs = [],
    hdrs = ["map_util.h"],
    deps = [],
)

cc_library(
    name = "math_limits",
    srcs = ["math_limits.cc"],
    hdrs = ["math_limits.h"],
    deps = ["//cpp-base",],
)

cc_library(
    name = "math_util",
    srcs = [],
    hdrs = ["math_util.h"],
    deps = [":math_limits",
            "//cpp-base",],
)

cc_library(
    name = "periodic_closure",
    srcs = ["periodic_closure.cc"],
    hdrs = ["periodic_closure.h"],
    deps = [":clock",
            "//cpp-base",],
)

cc_library(
    name = "random",
    srcs = ["random.cc"],
    hdrs = ["random.h"],
    deps = ["//cpp-base",
            "//cpp-base/hash",],
)

cc_library(
    name = "stl_util",
    srcs = [],
    hdrs = ["stl_util.h"],
    deps = [],
)

cc_library(
    name = "sysinfo",
    srcs = ["sysinfo.cc"],
    hdrs = ["sysinfo.h"],
    deps = ["//cpp-base",
            "//cpp-base/string:stringpiece",],
)

cc_library(
    name = "triple",
    srcs = [],
    hdrs = ["triple.h"],
    deps = [],
)
