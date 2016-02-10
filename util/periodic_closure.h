#ifndef CPP_BASE_UTIL_PERIODIC_CLOSURE_H_
#define CPP_BASE_UTIL_PERIODIC_CLOSURE_H_

#include <glog/logging.h>
#include <condition_variable>   // NOLINT
#include <functional>
#include <thread>               // NOLINT
#include "cpp-base/callback.h"
#include "cpp-base/integral_types.h"
#include "cpp-base/macros.h"
#include "cpp-base/util/clock.h"

namespace cpp_base {

// Runs the given closure at certain periods. Tries to fire the timer accurately
// with the given period even if a run takes long; if it takes longer than the
// period, 1+ runs are skipped. The first run fires right at construction time.
// The closure and the period value cannot be modified after construction; to
// modify, delete and create a new instance instead.
class PeriodicClosure {
  public:
    // Takes ownership of closure. Does not take ownership of clock.
    PeriodicClosure(Closure* closure, int64 period_millisec, Clock* clock);
    ~PeriodicClosure();

  private:
    typedef std::chrono::steady_clock clock;
    void Run();

    std::unique_ptr<Closure> closure_;
    const clock::duration period_;
    std::thread thread_;
    std::condition_variable cond_var_;
    Clock* clock_;
    bool should_stop_;
    bool did_stop_;

    DISALLOW_COPY_AND_ASSIGN(PeriodicClosure);
};

}  // namespace cpp_base

#endif  // CPP_BASE_UTIL_PERIODIC_CLOSURE_H_
