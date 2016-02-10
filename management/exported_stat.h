#ifndef CPP_BASE_MANAGEMENT_EXPORTED_STAT_H_
#define CPP_BASE_MANAGEMENT_EXPORTED_STAT_H_

#include <glog/logging.h>
#include <atomic>
#include <memory>
#include <sstream>
#include <string>
#include "cpp-base/management/exportee.h"
#include "cpp-base/management/global_exporter.h"
#include "cpp-base/macros.h"
#include "cpp-base/mutex.h"

// See the documentation in exportee.h.

namespace cpp_base {

// Abstract class to hold the common code for different exported types below.
class AbstractExportedStat : public Exportee {
  public:
    // Does not take ownership of the pointers and they must remain valid.
    AbstractExportedStat(const std::string& name) : Exportee(name) {
        GlobalExporter::ExportStat(this);
    }

    virtual ~AbstractExportedStat() {
        if (!GlobalExporter::UnexportStat(this)) {
            LOG(ERROR) << "No such stat in the global list: " << Name();
        }
    }

    virtual std::string GetValue() = 0;
    virtual bool ResetValue() = 0;

    // There is not SetValue(value) for stat variables -- only ResetValue().
    bool SetValue(const std::string& value) override { return SeppukuDie(); }
};

// This is to capture at compile time the error of trying to instantiate a
// ExportedStatVariable<T> for non-primitive types as T. If you hit this,
// you probably need ExportedStatCallback, not ExportedStatVariable.
template <typename T>
class ExportedStatVariable;

// Represents a primitive stat variable that we'd like to export.
// Type T must support "ostream << T" operation.
template <typename T>
class ExportedStatVariable<std::atomic<T>> : public AbstractExportedStat {
  public:
    // Does not take ownership of the pointer and it must remain valid.
    ExportedStatVariable(
            const std::string& name, std::atomic<T>* variable, bool can_reset)
            : AbstractExportedStat(name),
              variable_(variable), can_reset_(can_reset) {}

    virtual ~ExportedStatVariable() {
        LOG(INFO) << "Unexprting " << Name() << "; last value: " << GetValue();
    }

    std::string GetValue() override {
        return Exportee::ToString<T>(variable_->load());
    }

    bool ResetValue() override {
        if (!can_reset_)
            return false;
        variable_->store(T());
        return true;
    }

  private:
    std::atomic<T>* variable_;  // Ownership not ours
    const bool can_reset_;

    DISALLOW_COPY_AND_ASSIGN(ExportedStatVariable);
};

//#define LEGACY_EXPORT
#ifdef LEGACY_EXPORT
// The legacy way for exporting a primitive variable: concurrency through mutex,
// rather than std::atomic. Not recommended; only exists for historic reasons.
// Type T must support "ostream << T" operation.
template <typename T>
class ExportedStatVariable : public AbstractExportedStat {
  public:
    // Does not take ownership of the pointers and they must remain valid.
    ExportedStatVariable(
            const std::string& name, T* variable, bool can_reset, Mutex* mutex)
            : AbstractExportedStat(name),
              variable_(variable), can_reset_(can_reset), mutex_(mutex) {}

    virtual ~ExportedStatVariable() {
        LOG(INFO) << "Unexprting " << Name() << "; last value: " << GetValue();
    }

    std::string GetValue() override {
        MutexLock lock(mutex_);
        return Exportee::ToString<T>(*variable_);
    }

    bool ResetValue() override {
        if (!can_reset_)
            return false;
        MutexLock lock(mutex_);
        *variable_ = T();
        return true;
    }

  private:
    T* variable_;   // Ownership not ours
    const bool can_reset_;
    Mutex* mutex_;  // Ownership not ours

    DISALLOW_COPY_AND_ASSIGN(ExportedStatVariable);
};
#endif

// Represents a more complex exported stat: one coming from a callback.
// Type T must support "ostream << T" operation.
template <typename T>
class ExportedStatCallback : public AbstractExportedStat {
  public:
    // Use this constructor for non-resetable stats.
    // Then, any call to ResetValue() attempts will get 'false' back.
    // get_callback: what returns the exported value.
    ExportedStatCallback(const std::string& name,
                         std::function<T()> get_callback)
                         : AbstractExportedStat(name),
                           get_callback_(get_callback) {
        // There is no 'reset' function given; the default is to return false.
        reset_callback_ = std::function<bool()>([]{ return false; });
    }

    // Use this constructor for resetable stats.
    // get_callback: what returns the exported value.
    // reset_callback: what resets the exported value.
    ExportedStatCallback(const std::string& name,
                         std::function<T()> get_callback,
                         std::function<void()> reset_callback)
                         : AbstractExportedStat(name),
                           get_callback_(get_callback),
                           __reset_callback(reset_callback) {
        reset_callback_ = std::function<bool()>([&]{
            __reset_callback();
            return true;
        });
    }

    virtual ~ExportedStatCallback() {
        LOG(INFO) << "Unexprting " << Name() << "; last value: " << GetValue();
    }

    std::string GetValue() override {
        return Exportee::ToString<T>(get_callback_());
    }

    bool ResetValue() override { return reset_callback_(); }

  private:
    // The callback to retrieve the exported value.
    std::function<T()> get_callback_;

    // We wrap the original reset callback given to us in another function --
    // 'reset_callback_' below. The original function, however, may be just an
    // in-line lambda function which will go out of scope and get destroyed, so
    // keep a copy of it in the class scope -- '__reset_callback' below.
    std::function<bool()> reset_callback_;
    std::function<void()> __reset_callback;

    DISALLOW_COPY_AND_ASSIGN(ExportedStatCallback);
};

}  // namespace cpp_base

#endif  // CPP_BASE_MANAGEMENT_EXPORTED_STAT_H_
