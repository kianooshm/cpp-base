package(default_visibility = ["//visibility:public"])

cc_library(
    name = "gtest",
#    srcs = glob(
#        ["src/*.cc"],
#        exclude = ["src/gtest-all.cc"]
#    ),
    srcs = ["src/gtest-death-test.cc",
            "src/gtest_main.cc",
            "src/gtest-printers.cc",
            "src/gtest-typed-test.cc",
            "src/gtest.cc",
            "src/gtest-filepath.cc",
            "src/gtest-port.cc",
            "src/gtest-test-part.cc",],
    hdrs = glob(["include/**/*.h"]),
    includes = [
        ".",
        "include"
    ],
    linkopts = ["-pthread"],
)

