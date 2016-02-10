#include "cpp-base/util/clock.h"

#include <sched.h>
#include <time.h>
#include <cmath>
#include <stdexcept>
#include "cpp-base/string/join.h"

using std::multiset;
using std::pair;
using std::string;

namespace cpp_base {

const int64 kNumMillisPerSecond = 1000ll;
const int64 kNumMicrosPerSecond = 1000000ll;
const int64 kNumNanosPerSecond = 1000000000ll;

const int64 kNumSecondsPerMinute = 60;
const int64 kNumSecondsPerHour = 60 * kNumSecondsPerMinute;
const int64 kNumSecondsPerDay = 24 * kNumSecondsPerHour;

const int64 kNumMillisPerMinute = kNumSecondsPerMinute * kNumMillisPerSecond;
const int64 kNumMillisPerHour = kNumSecondsPerHour * kNumMillisPerSecond;
const int64 kNumMillisPerDay = kNumSecondsPerDay * kNumMillisPerSecond;

RealClock* Clock::real_clock_ = new RealClock();

////////////////////////////////// RealClock ///////////////////////////////////

double RealClock::Now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.;
}

void RealClock::Sleep(double t) {
    struct timespec ts;
    ts.tv_sec = std::floor(t);
    ts.tv_nsec = (t - ts.tv_sec) * 1000000000ll;
    nanosleep(&ts, NULL);
}

void RealClock::WaitOnCondVar(std::condition_variable* cond_var,
                              std::unique_lock<std::mutex>* lock,
                              int64 microseconds) {
    cond_var->wait_for(*lock, std::chrono::microseconds(microseconds));
}

void RealClock::NotifyCondVar(std::condition_variable* cond_var) {
    cond_var->notify_one();
}

//////////////////////////////// SimulatedClock ////////////////////////////////

double SimulatedClock::Now() {
    MutexLock lock(&mutex_);
    return now_;
}

void SimulatedClock::Sleep(double t) {
    double target_t = Now() + t;
    while (Now() < target_t) {
        sched_yield();
    }
}

void SimulatedClock::WaitOnCondVar(std::condition_variable* cond_var,
                                   std::unique_lock<std::mutex>* lock,
                                   int64 microseconds) {
    {
        MutexLock lock(&mutex_);
        pair<double, std::condition_variable*> p(
                now_ + microseconds / 1000000., cond_var);
        CHECK(!ContainsKey(events_, p));
        events_.insert(p);
    }
    cond_var->wait(*lock);
}

void SimulatedClock::NotifyCondVar(std::condition_variable* cond_var) {
    MutexLock lock(&mutex_);
    multiset<pair<double, std::condition_variable*>>::iterator it, it2;
    for (it = events_.begin(); it != events_.end(); ) {
        if (it->second == cond_var) {
            it2 = it++;
            events_.erase(it2);
            cond_var->notify_one();
            LOG(INFO) << "notified";
        } else {
            ++it;
        }
    }
    cond_var->notify_one();
}

void SimulatedClock::AdvanceTime(double t) {
    MutexLock lock(&mutex_);
    CHECK_GE(t, 0);
    now_ += t;
    multiset<pair<double, std::condition_variable*>>::iterator it;
    it = events_.begin();
    while (it != events_.end() && it->first <= now_) {
        it->second->notify_one();
        events_.erase(it);
        it = events_.begin();
    }
}

////////////////////////////// Utility functions ///////////////////////////////

time_t mkgmtime(const struct tm& tm) {
  static const int month_day[12] =
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  time_t tm_year = tm.tm_year;
  time_t tm_mon = tm.tm_mon;
  time_t tm_mday = tm.tm_mday;
  time_t tm_hour = tm.tm_hour;
  time_t tm_min = tm.tm_min;
  time_t tm_sec = tm.tm_sec;
  time_t month = tm_mon % 12;
  time_t year = tm_year + tm_mon / 12;
  if (month < 0) {
    month += 12;
    --year;
  }
  const time_t year_for_leap = (month > 1) ? year + 1 : year;
  return tm_sec + 60 * (tm_min + 60 * (tm_hour + 24 *
            (month_day[month] + tm_mday - 1 + 365 * (year - 70) +
            (year_for_leap - 69) / 4 - (year_for_leap - 1) / 100 +
            (year_for_leap + 299) / 400)));
}

uint64 strptime_64(const char* input, const char* format) {
  if (!input || !strlen(input))
    throw std::out_of_range("No input string");

  uint32_t ms = 0;
  struct tm tm = {0, };
  const char* p = strptime(input, format, &tm);
  if (p == nullptr)
    throw std::out_of_range("Invalid input string for format");
  if (*p == '.') {
    // There are milliseconds
    if (strlen(p) < 2)
      throw std::out_of_range("Not enough characters left in input for ms");

    if (sscanf(p, ".%3d", &ms) != 1)
      throw std::out_of_range("Milliseconds not found in input string");
    p += 4;
  }
  uint64_t ret = mkgmtime(tm);
  if (ret < 0)
    throw std::out_of_range("Invalid return from mkgmtime");
  return ret * 1000 + ms;
}

// static
string Clock::GmTime(double t_secs, const char* format) {
  time_t seconds = std::floor(t_secs);
  struct tm tm;
  if (!gmtime_r(&seconds, &tm))
    return "";
  char buf[128] = {0};
  if (strftime(buf, 127, format, &tm) <= 0)
    return "";
  string out(buf);
  size_t pos = out.find("%.");
  if (pos != string::npos) {
    int millisecs = std::floor((t_secs - seconds) * 1000);
    out.replace(pos, 2, StrCat(millisecs));
  }
  return out;
}

uint64_t Clock::FromGmTime(const string& input, const char* format) {
  return strptime_64(input.c_str(), format);
}

#if 0
TEST_F(TimerTest, Strftime) {
  Timestamp t(1294356842114);
  string timestr = t.GmTime("%Y-%m-%d %H:%M:%S.%. %Z");
  EXPECT_EQ("2011-01-06 23:34:02.114 GMT", timestr);
  EXPECT_EQ(1294356842114, Timestamp::FromGmTime(timestr, "%Y-%m-%d %H:%M:%S"));
}
#endif

}  // namespace cpp_base
