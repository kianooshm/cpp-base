package(default_visibility = ["//visibility:public"])

cc_library(
    name = "string",
    srcs = [],
    hdrs = [],
    deps = [":join",
            ":numbers",
            ":split",
            ":stringpiece",
            ":stringprintf",
            ":strtoint",],
)

cc_library(
    name = "join",
    srcs = ["join.cc"],
    hdrs = ["join.h"],
    deps = [":numbers",
            ":stringpiece",
            "//cpp-base",
            "//cpp-base/util:ascii_ctype",
            "//cpp-base/util:stl_util",],
)

cc_library(
    name = "numbers",
    srcs = ["numbers.cc"],
    hdrs = ["numbers.h"],
    deps = [":stringprintf",
            ":strtoint",
            "//cpp-base",
            "//cpp-base/util:ascii_ctype",],
)

cc_library(
    name = "split",
    srcs = ["split.cc",],
    hdrs = ["split.h",],
    deps = [":stringpiece",
            "//cpp-base",],
)

cc_library(
    name = "stringpiece",
    srcs = ["stringpiece.cc"],
    hdrs = ["stringpiece.h"],
    deps = ["//cpp-base"],
)

cc_library(
    name = "stringprintf",
    srcs = ["stringprintf.cc"],
    hdrs = ["stringprintf.h"],
    deps = ["//cpp-base"],
)

cc_library(
    name = "strtoint",
    srcs = ["strtoint.cc"],
    hdrs = ["strtoint.h"],
    deps = ["//cpp-base"],
)
