package(default_visibility = ["//visibility:public"])

cc_library(
    name = "management",
    srcs = ["global_exporter.cc",],
    hdrs = ["exported_config.h",
            "exported_stat.h",
            "global_exporter.h",],
    deps = ["//cpp-base",
            "//cpp-base/string:join",
            "//cpp-base/string:numbers",
            "//cpp-base/string:stringprintf",
            "//cpp-base/util:map_util",],
)

cc_test(
    name = "export_test",
    srcs = ["export_test.cc",],
    deps = [":management",
            "//cpp-base",
            "//cpp-base/string:join",
            "//cpp-base/gtest",],
    timeout = "short",
)
