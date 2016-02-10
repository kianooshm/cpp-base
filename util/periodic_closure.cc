#include "cpp-base/util/periodic_closure.h"

namespace cpp_base {

PeriodicClosure::PeriodicClosure(
        Closure* closure, int64 period_millisec, Clock* clock)
        : closure_(closure),
          period_(std::chrono::milliseconds(period_millisec)),
          clock_(clock),
          should_stop_(false),
          did_stop_(false) {
    CHECK_GT(period_millisec, 0);
    CHECK(closure_->IsRepeatable());
    LOG_IF(WARNING, !clock::is_steady) << "clock is not steady";
    thread_ = std::thread(&PeriodicClosure::Run, this);
}

PeriodicClosure::~PeriodicClosure() {
    // There is a corner case where in here we "notify" the cond var, but then
    // the Run() thread goes into "wait"-ing on the cond var and never gets
    // notified. So make sure we do call a "notify" AFTER any "wait". Note that
    // "notify" on a non-waited-on cond var is harmless.
    should_stop_ = true;
    do {
        clock_->NotifyCondVar(&cond_var_);   // Interrupt the sleep in Run()
        sched_yield();
    } while (!did_stop_);
    if (thread_.joinable())
        thread_.join();
}

void PeriodicClosure::Run() {
    // We need a locked mutex for cond_var_.
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);

    clock::time_point next_run_time = clock::now();
    while (!should_stop_) {
        closure_->Run();
        // Did the run take so long that we missed a period?
        clock::time_point now = clock::now();
        while (next_run_time < now) {
            next_run_time += period_;
        }
        // Sleep until the next fire ... or until we are told to stop.
        int64 microsecs = std::chrono::duration_cast<std::chrono::microseconds>(
                            next_run_time - now).count();
        clock_->WaitOnCondVar(&cond_var_, &lock, microsecs);
    }
    did_stop_ = true;
}

}  // namespace cpp_base
