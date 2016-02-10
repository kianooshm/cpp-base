#ifndef CPP_BASE_MANAGEMENT_GLOBAL_EXPORTER_H_
#define CPP_BASE_MANAGEMENT_GLOBAL_EXPORTER_H_

#include <memory>
#include <map>
#include <string>
#include "cpp-base/integral_types.h"
#include "cpp-base/macros.h"
#include "cpp-base/mutex.h"

/*
 See the documentation in exportee.h on how to create exported stat variables
 and config params.

 Exported stats and configs, though maintained in two separate sets, are
 contained managed by the same, global exporter: GlobalExporter below.
 This singleton class provides API to retrieve/set/reset exportees by name.

 You can write your own user interface on top of this exporter, e.g., via
 command line.
 A nice and pretty HTTP interface is provided in cpp-base/http/http_server.cc:
 it exposes http:server:port/varz for stats and /configz for config parameters.
*/

namespace cpp_base {

class Exportee;

// This singleton contains and manages all exported stats and configs.
// Note: You probably don't need to deal with this class in your own code.
// You should work with ExportedVariable
//       and ExportedCallback, which add/remove themselves to this global set.
class GlobalExporter {
    typedef std::string string;  // Instead of the deprecated "using std::string" in a header file
  public:
    GlobalExporter();
    ~GlobalExporter() {}

    // Return the global VariableExporter instance.
    static GlobalExporter* Instance();

    // Convenience methods to add/remove exportees. See Add/Remove() below.
    static void ExportStat(Exportee* e)   { Instance()->Add(e, true);  }
    static void ExportConfig(Exportee* e) { Instance()->Add(e, false); }
    static bool UnexportStat(Exportee* e) {
        return Instance()->Remove(e, true);
    }
    static bool UnexportConfig(Exportee* e) {
        return Instance()->Remove(e, false);
    }

    // Renders and returns all or one of the contained exportees: lines in the form "var=value\n".
    string RenderStat(const string& name)   { return RenderOne(name, true);  }
    string RenderConfig(const string& name) { return RenderOne(name, false); }
    string RenderAllStats()   { return RenderAll(true);  }
    string RenderAllConfigs() { return RenderAll(false); }

    // Returns only the value of a stat or config variable. Returns empty string if not found.
    string GetStatValue(const string& name)   { return GetOne(name, true); }
    string GetConfigValue(const string& name) { return GetOne(name, false); }

    // Resets all or one of the exported stats; not applicable to configs.
    // Returns a response message explaining success or failure.
    string ResetStat(const string& name);
    string ResetAllStats();

    // Sets the value of an exported config; not applicable to stats.
    // Returns a response message explaining success or failure.
    string SetConfig(const string& name, const string& value);

  private:
    // Add/Remove an exportee to/from the global set: 'is_stat' determines
    // whether it is a stat or config, to go to the respective stat/config set.
    // Ownership of the pointers is not taken and they must remain valid.
    // In Add(), if an exportee with the same name exists, i's overwritten.
    // In Remove(), if the exportee is not Add()-ed before, false is returned.
    void Add(Exportee* var, bool is_stat);
    bool Remove(Exportee* var, bool is_stat);

    // Renders all or one of the contained exportees as text and returns it.
    string RenderOne(const string& name, bool is_stat);
    string RenderAll(bool is_stat);

    // Returns only the value of a stat or config variable.
    string GetOne(const string& name, bool is_stat);

    Mutex mutex_;
    std::map<string /*name*/, Exportee*> exported_stats_;
    std::map<string /*name*/, Exportee*> exported_configs_;

    const int64 start_time_secs_;  // For showing system uptime
    std::unique_ptr<Exportee> uptime_export_callback_;

    DISALLOW_COPY_AND_ASSIGN(GlobalExporter);
};

}  // namespace cpp_base

#endif  // CPP_BASE_MANAGEMENT_GLOBAL_EXPORTER_H_
