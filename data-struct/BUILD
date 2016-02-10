package(default_visibility = ["//visibility:public"])

cc_library(
    name = "count_min_sketch",
    srcs = [],
    hdrs = ["count_min_sketch.h",],
    deps = ["//cpp-base",
            "//cpp-base/hash",],
)

cc_test(
    name = "count_min_sketch_test",
    srcs = ["count_min_sketch_test.cc",],
    deps = [":count_min_sketch",
            "//cpp-base/gtest",],
    timeout = "short",
)

cc_library(
    name = "lru_map",
    srcs = [],
    hdrs = ["lru_map.h",],
    deps = ["//cpp-base",
            "//cpp-base/hash",
            "//cpp-base/util:map_util",],
)

cc_library(
    name = "lru_set",
    srcs = [],
    hdrs = ["lru_set.h",],
    deps = ["//cpp-base",
            "//cpp-base/hash",
            "//cpp-base/util:map_util",],
)

cc_test(
    name = "lru_map_test",
    srcs = ["lru_map_test.cc",],
    deps = [":lru_map",
            "//cpp-base/gtest",],
    timeout = "short",
)

cc_library(
    name = "producer_consumer_queue",
    srcs = [],
    hdrs = ["producer_consumer_queue.h",],
    deps = ["//cpp-base",],
)

cc_library(
    name = "ring_buffer",
    srcs = [],
    hdrs = ["ring_buffer.h",],
    deps = ["//cpp-base",],
)

cc_test(
    name = "ring_buffer_test",
    srcs = ["ring_buffer_test.cc",],
    deps = [":ring_buffer",
            "//cpp-base/gtest",],
    timeout = "short",
)

cc_library(
    name = "vector_map",
    srcs = [],
    hdrs = ["vector_map.h",],
    deps = ["//cpp-base",
            "//cpp-base/hash",
            "//cpp-base/util",],
)

cc_library(
    name = "zvector",
    srcs = [],
    hdrs = ["zvector.h",],
    deps = ["//cpp-base",],
)
