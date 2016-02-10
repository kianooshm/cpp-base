#ifndef CPP_BASE_UTIL_CLOCK_H_
#define CPP_BASE_UTIL_CLOCK_H_

#include <glog/logging.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <condition_variable>   // NOLINT
#include <set>
#include <string>
#include <utility>

#include "cpp-base/integral_types.h"
#include "cpp-base/macros.h"
#include "cpp-base/mutex.h"
#include "cpp-base/util/map_util.h"

namespace cpp_base {

extern const int64 kNumMillisPerSecond;
extern const int64 kNumMicrosPerSecond;
extern const int64 kNumNanosPerSecond;

extern const int64 kNumSecondsPerMinute;
extern const int64 kNumSecondsPerHour;
extern const int64 kNumSecondsPerDay;

extern const int64 kNumMillisPerMinute;
extern const int64 kNumMillisPerHour;
extern const int64 kNumMillisPerDay;

class RealClock;

// Generic clock interface to represent either a real or a simulated clock.
class Clock {
  public:
    Clock() { }
    virtual ~Clock() { }

    // Returns the number of seconds since epoch.
    virtual double Now() = 0;

    // Sleeps for the given number of seconds.
    virtual void Sleep(double t) = 0;

    // Waits on the given condition variable until either "notify"-ed or the
    // timeout is reached. Does not take ownership of the pointer, however,
    // the condition variable MUST remain valid until the current wait is
    // notified.
    virtual void WaitOnCondVar(std::condition_variable* cond_var,
                               std::unique_lock<std::mutex>* lock,
                               int64 microseconds) = 0;

    // Send a "notify" to whoever waiting on the given condition variable.
    virtual void NotifyCondVar(std::condition_variable* cond_var) = 0;

    static RealClock* GlobalRealClock() { return real_clock_; }

    // Renders the given timestamp using strftime().
    // This also accepts another formatting conversion "%." which is replaced
    // with the timestamp's fraction of a second,  which is useful in something
    // like:
    //   Timestamp().strftime("%H:%M:%S.%.") --> 12:15:05.114
    // Return empty string upon error.
    static std::string GmTime(double t_secs,
                              const char* format = "%Y-%m-%d %H:%M:%S.%.");

    static uint64 FromGmTime(const std::string& input,
                             const char* format = "%Y-%m-%d %H:%M:%S");

  private:
    static RealClock* real_clock_;
    DISALLOW_COPY_AND_ASSIGN(Clock);
};

// RealClock uses standard system calls to get the current time, wait and sleep.
// This class is thread-safe.
class RealClock : public Clock {
  public:
    RealClock() { }
    virtual ~RealClock() { }
    double Now() override;
    void Sleep(double t) override;
    void WaitOnCondVar(std::condition_variable* cond_var,
                       std::unique_lock<std::mutex>* lock,
                       int64 microseconds) override;
    void NotifyCondVar(std::condition_variable* cond_var) override;

  private:
    DISALLOW_COPY_AND_ASSIGN(RealClock);
};

// SimulatedClock only progresses manually through AdvanceTime(). Supports wait
// and sleep like RealClock, but with simulated time. This class is thread safe.
class SimulatedClock : public Clock {
  public:
    SimulatedClock() : now_(0) { }
    explicit SimulatedClock(double now) : now_(now) { }
    virtual ~SimulatedClock() { }
    double Now() override;
    void Sleep(double t) override;
    void WaitOnCondVar(std::condition_variable* cond_var,
                       std::unique_lock<std::mutex>* lock,
                       int64 microseconds) override;
    void NotifyCondVar(std::condition_variable* cond_var) override;
    // Adds 't' to the current time.
    void AdvanceTime(double t);

  private:
    Mutex mutex_;
    double now_;  // in seconds
    std::multiset<std::pair<double, std::condition_variable*>> events_;
    DISALLOW_COPY_AND_ASSIGN(SimulatedClock);
};

inline double Now() {
    return Clock::GlobalRealClock()->Now();
}

// Returns ms since epoch given an input time string and format.
// Throws out_of_range on error
uint64_t strptime_64(const char* input, const char* format);

// Make a time_t (seconds since epoch) out a struct tm.
// This is a replacement for mktime that assumes GMT.
// Returns negative on failure.
time_t mkgmtime(const struct tm& tm);

}  // namespace cpp_base

#endif  // CPP_BASE_UTIL_CLOCK_H_
