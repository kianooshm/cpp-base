package(default_visibility = ["//visibility:public"])

cc_library(
    name = "pcqueue",
    srcs = [],
    hdrs = ["abstract_blocking_pc_queue.h",
            "abstract_nonblocking_pc_queue.h",
            "blocking_queue_by_semaphore.h",
            "blocking_queue_by_spinlock.h",
            "mpmc_queue_by_mutex.h",
            "spsc_queue_by_drdobbs.h",
            "spsc_queue_by_folly.h",],
    deps = ["//cpp-base",
            "//cpp-base/data-struct:ring_buffer",],
)
