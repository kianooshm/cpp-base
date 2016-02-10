package(default_visibility = ["//visibility:public"])

cc_library(
    name = "membership_manager",
    srcs = ["membership_manager.cc"],
    hdrs = ["membership_manager.h",],
    deps = [":zk_client",
            "//cpp-base",
            "//cpp-base/string:join",],
)

cc_library(
    name = "zk_client",
    srcs = ["zk_client.cc"],
    hdrs = ["zk_client.h",],
    deps = [],
)
