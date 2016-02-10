#ifndef CPP_BASE_MANAGEMENT_EXPORTED_CONFIG_H_
#define CPP_BASE_MANAGEMENT_EXPORTED_CONFIG_H_

#include <glog/logging.h>
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include "cpp-base/integral_types.h"
#include "cpp-base/macros.h"
#include "cpp-base/management/exportee.h"
#include "cpp-base/management/global_exporter.h"
#include "cpp-base/mutex.h"

namespace cpp_base {

// Abstract class to hold the common code for different exported types below.
class AbstractExportedConfig : public Exportee {
  public:
    // Does not take ownership of the pointers and they must remain valid.
    AbstractExportedConfig(const std::string& name) : Exportee(name) {
        GlobalExporter::ExportConfig(this);
    }

    virtual ~AbstractExportedConfig() {
        if (!GlobalExporter::UnexportConfig(this)) {
            LOG(ERROR) << "No such config in the global list: " << Name();
        }
    }

    // Returns the current value of the exported config parameter as string.
    virtual std::string GetValue() = 0;

    // Sets the value of the config parameter to the given string. Returns true iff successful.
    virtual bool SetValue(const std::string& value) = 0;

    // There is not ResetValue() for config parameters -- only SetValue(value).
    bool ResetValue() override { return SeppukuDie(); }
};

// This is to capture at compile time the error of trying to instantiate a
// ExportedConfigParameter<T> for non-primitive types as T. If you hit this,
// you probably need ExportedConfigCallback, not ExportedConfigParameter.
template <typename T>
class ExportedConfigParameter;

// Represents a primitive config parameter that we'd like to export -- NOT for e.g. T=std::string.
// Type T must support "ostream << T" operation.
template <typename T>
class ExportedConfigParameter<std::atomic<T>> : public AbstractExportedConfig {
  public:
    // Does not take ownership of the pointer and it must remain valid.
    // validation_callback: a callback that returns true/false on a given value,
    //                      used by SetValue() to verify values before setting.
    ExportedConfigParameter(
            const std::string& name,
            std::atomic<T>* parameter,
            std::function<bool(T)> validation_callback)
            : AbstractExportedConfig(name),
              parameter_(parameter),
              validation_callback_(validation_callback) {}

    // Returns the current value of the exported config parameter as string.
    std::string GetValue() override {
        return Exportee::ToString<T>(parameter_->load());
    }

    // Sets the value of the config parameter to the given string. Returns true iff successful.
    bool SetValue(const std::string& value) override {
        T v;
        if (!Exportee::ParseValue<T>(value, &v))
            return false;
        if (!validation_callback_(v))
            return false;
        parameter_->store(v);
        return true;
    }

  private:
    std::atomic<T>* parameter_;  // Ownership not ours
    std::function<bool(T)> validation_callback_;

    DISALLOW_COPY_AND_ASSIGN(ExportedConfigParameter);
};

/*
// We are NOT going to have ExportedConfigParameter<T> for arbitrary
// (non-primitive) types as T. If trying to make one, it will be captured as a
// compile-time error since the following (invalid) class is instantiated.
template <typename T>
class ExportedConfigParameter : public AbstractExportedConfig {};
*/

// Represents a more complex exported config: one done through a callback.
// Type T must support "ostream << T" operation.
template <typename T>
class ExportedConfigCallback : public AbstractExportedConfig {
  public:
    // name: the name of the exported config.
    // get_callback: callback for getting the value; should return a string.
    // set_callback: callback for setting the value; takes a string and returns
    //               a boolean indicating success (e.g., in/valid value).
    // Ownership of callback pointers is taken.
    explicit ExportedConfigCallback(const std::string& name,
                                    std::function<T()> get_callback,
                                    std::function<bool(T)> set_callback)
                                    : AbstractExportedConfig(name),
                                      get_callback_(get_callback),
                                      set_callback_(set_callback) {}

    // Returns the current value of the exported config parameter as string.
    std::string GetValue() override {
        return Exportee::ToString<T>(get_callback_());
    }

    // Sets the value of the config parameter to the given string. Returns true iff successful.
    // This function uses Exportee::ParseValue<T>() which is (intentionally) ONLY defined for
    // primitive types, therefore it is template-specialized for other types below (currently
    // only std::string).
    inline bool SetValue(const std::string& value) {
        T v;
        return Exportee::ParseValue<T>(value, &v) ? set_callback_(v) : false;
    }

  private:
    std::function<T()> get_callback_;
    std::function<bool(T)> set_callback_;

    DISALLOW_COPY_AND_ASSIGN(ExportedConfigCallback);
};

// Specialization of ExportedConfigCallback<T>::SetValue for non-primitive types.
template <>
inline bool ExportedConfigCallback<std::string>::SetValue(const std::string& value) {
    return set_callback_(value);
}

}  // namespace cpp_base

#endif  // CPP_BASE_MANAGEMENT_EXPORTED_CONFIG_H_
