package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cuckoo-filter",
    srcs = ["util.cc"],
    hdrs = ["cuckoo_filter.h",
            "single_table.h",
            "util.h",],
    deps = ["//cpp-base",
            "//cpp-base/hash",],
)

cc_test(
    name = "cuckoo_filter_test",
    srcs = ["cuckoo_filter_test.cc",],
    deps = [":cuckoo-filter",
            "//cpp-base/util:map_util",
            "//cpp-base/gtest",],
    timeout = "short",
)
