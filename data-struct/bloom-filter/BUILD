package(default_visibility = ["//visibility:public"])

cc_library(
    name = "bloom-filter",
    srcs = ["bloom_filter.cc",
            "expandable_bloom_filter.cc",],
    hdrs = ["bloom_filter.h",
            "expandable_bloom_filter.h",],
    deps = ["//cpp-base",
            "//cpp-base/hash",],
)

cc_test(
    name = "bloom_filter_test",
    srcs = ["bloom_filter_test.cc",],
    deps = [":bloom-filter",
            "//cpp-base/util",
            "//cpp-base/gtest",],
    timeout = "short",
)

cc_test(
    name = "expandable_bloom_filter_test",
    srcs = ["expandable_bloom_filter_test.cc",],
    deps = [":bloom-filter",
            "//cpp-base/util",
            "//cpp-base/gtest",],
    timeout = "short",
)
