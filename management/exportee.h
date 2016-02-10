#ifndef CPP_BASE_MANAGEMENT_EXPORTEE_H_
#define CPP_BASE_MANAGEMENT_EXPORTEE_H_

#include <glog/logging.h>
#include <atomic>
#include <string>
#include "cpp-base/macros.h"
#include "cpp-base/string/numbers.h"

/*
 You can export a stat variable, a config parameter, or a stat/config callback
 using the classes provided in this package. The classes are as follows:

   Exportee
      |
      +-- AbstractExportedStat
      |      |
      |      +-- ExportedStatVariable
      |      |
      |      +-- ExportedStatCallback
      |
      +-- AbstractExportedConfig
             |
             +-- ExportedConfigParameter
             |
             +-- ExportedConfigCallback

 There is no technical difference between exported stats and configs except that
 stats can only be retrieved and reset (but not set to a given value) whereas
 configs can be retrieved and set to given values (but not reset). Both exported
 stats and configs are managed by the same exporter: see GlobalExporter.

 To have a stat/config exported --that is, being added to the global export set
 and accessed via e.g. http-- all you need is to make the relevant ExportedXXX
 object; its constructor will automatically add itself to the global export set.
 You can unexport a variable too. You can also reset the value of the variable.

  For actual usage, see the unit tests. The following is just to give an idea.

 -- Sample usage with a primitive-variable exported stat:

  // std::atomic simplifies concurrent access, in most cases in a lock-free way.
  // To export raw primitive variables (possibly protected by your own mutex),
  // set LEGACY_EXPORT compile flag.
  std::atomic<int> num_failed_requests;

  ExportedStatVariable<std::atomic<int>> e(
        "number_of_failed_requests",    // Name of the exportee
        &num_failed_requests,           // Pointer to the variable
        true);                          // Whether to allow resetting this stat

 You'd typically call the above line in the constructor of a class or in main().

 Usage of primitive-variable exported config parameters is the same way, except
 that they take a 'validator' as well -- typically an in-line lambda function.

 -- Sample usage with a callback-based exported variable:

  class MyServer {
      int64 TotalRequests() {           // Callback to compute the value
          int64 n = 0;
          for (auto& i : all_threads)
              n += t.NumRequests();     // Of course this must be thread safe
          return n;
      }
      void ResetTotalRequests() {       // Callback to reset the value
          for (auto& t : all_threads)
              t.ResetNumRequests();     // This must be thread safe too
      }
      // This is the version *with* a reset callback. There is also one without.
      ExportedStatCallback e(
            "total_requests_by_all_threads",
            std::bind<int64>(&MyServer::TotalRequests, this),
            std::bind<int64>(&MyServer::ResetTotalRequests, this));
  };

 Usage of callback-based exported config parameters is the same way except that
 a 'set' callback must be provided instead of 'reset'.
*/

namespace cpp_base {

// Abstract class which is the parent of all exported values.
class Exportee {
  public:
    explicit Exportee(const std::string& name) : name_(name) {}
    virtual ~Exportee() {}

    virtual std::string Name() const { return name_; }

    virtual std::string GetValue() = 0;
    virtual bool ResetValue() = 0;
    virtual bool SetValue(const std::string& value) = 0;

    // Exported stat variables can be 'reset' (but not 'set' to given values).
    // Exported conf variables can be 'set' to given values (but not 'reset').
    // This is a helper function for error-checking behavior with inapplicable
    // set/reset calls. The name (Seppuku) is from the Samurai doctrine.
    virtual bool SeppukuDie() {
        CHECK(false) << "I shall die with a suicide of honor for I have tried "
                     << "a forbidden operation on exportee '" << name_ << "'";
        return false;
    }

    // Helper function for converting values to string.
    // Type T must support "ostream << T" operation.
    template <class T>
    inline static std::string ToString(const T& value) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    // Helper function for parsing values from string. The template type is
    // specialized below for all primitive types. For unsupported types as T,
    // you'll get a compile error.
    template <class T>
    inline static bool ParseValue(const std::string& str, T* value);


  private:
    const std::string name_;

    DISALLOW_COPY_AND_ASSIGN(Exportee);
};

////////////////// Implementation of Exportee::ParseValue() ///////////////////

// Specializations of ExportedConfig::ParseValue() for the supported types is
// going to be quite similar so use this macro:
#define DEFINE_PARSE_CASE(TYPE, CONVERTER_FUNC)                         \
template <>                                                             \
inline bool Exportee::ParseValue(const std::string& str, TYPE* value) { \
    /* We don't want 'value' touched if 'str' is invalid */             \
    TYPE temp;                                                          \
    if (!CONVERTER_FUNC(str, &temp))                                    \
        return false;                                                   \
    *value = temp;                                                      \
    return true;                                                        \
}

DEFINE_PARSE_CASE(int32, safe_strto32);
DEFINE_PARSE_CASE(uint32, safe_strtou32);
DEFINE_PARSE_CASE(int64, safe_strto64);
DEFINE_PARSE_CASE(uint64, safe_strtou64);
DEFINE_PARSE_CASE(float, safe_strtof);
DEFINE_PARSE_CASE(double, safe_strtod);

#undef DEFINE_PARSE_CASE

// The absence of ParseValue<std::string> and other ParseValue<T> is intentional so that one gets
// a compiler error when trying to create ExportedConfigParameter<T> for non-primitive types.

}  // namespace cpp_base

#endif   // CPP_BASE_MANAGEMENT_EXPORTEE_H_
