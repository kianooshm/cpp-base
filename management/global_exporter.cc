#include "cpp-base/management/global_exporter.h"

#include <glog/logging.h>
#include <time.h>

#include "cpp-base/management/exported_config.h"
#include "cpp-base/management/exported_stat.h"
#include "cpp-base/string/join.h"
#include "cpp-base/string/stringprintf.h"
#include "cpp-base/util/map_util.h"

using std::map;
using std::pair;
using std::string;

namespace {

cpp_base::GlobalExporter global_exporter;

string FormatTime(int64 secs) {
    return cpp_base::StringPrintf(
            "%dd:%02d:%02d:%02d",
            secs / 86400,           // day
            (secs % 86400) / 3600,  // hour
            (secs % 3600) / 60,     // minute
            secs % 60);             // second
}

}  // namespace

namespace cpp_base {

// static
GlobalExporter* GlobalExporter::Instance() {
    return &global_exporter;
}

GlobalExporter::GlobalExporter()
        : start_time_secs_(time(NULL)),
          uptime_export_callback_(new ExportedStatCallback<string>(
                "system_uptime",
                std::function<string()>( [&]() {
                    return FormatTime(time(NULL) - start_time_secs_);
                }))) {}

void GlobalExporter::Add(Exportee* e, bool is_stat) {
    MutexLock lock(&mutex_);
    const string& name = e->Name();
    if (is_stat) {
        CHECK(!ContainsKey(exported_stats_, name)) << "Duplicate stat registration: " << name;
        exported_stats_[name] = e;
        LOG(INFO) << "Exported stat " << name;
    } else {
        CHECK(!ContainsKey(exported_configs_, name)) << "Duplicate stat registration: " << name;
        exported_configs_[name] = e;
        LOG(INFO) << "Exported config " << name;
    }
}

bool GlobalExporter::Remove(Exportee* e, bool is_stat) {
    MutexLock lock(&mutex_);
    map<string, Exportee*>& map = is_stat ? exported_stats_ : exported_configs_;
    bool removed = (map.erase(e->Name()) > 0);
    if (removed) {
        LOG(INFO) << "Unexported " << (is_stat ? "stat '" : "config '") << e->Name() << "'";
    } else {
        LOG(WARNING) << (is_stat ? "No stat '" : "No config '") << e->Name() << "' to unexport";
    }
    return removed;
}

string GlobalExporter::RenderOne(const string& name, bool is_stat) {
    MutexLock lock(&mutex_);
    Exportee* e = FindWithDefault(
            is_stat ? exported_stats_ : exported_configs_, name, nullptr);
    return (e != nullptr)
            ? StrCat(e->Name(), "=", e->GetValue(), "\n")
            : StrCat("unknown ", is_stat ? "stat: " : "config: ", name, "\n");
}

string GlobalExporter::RenderAll(bool is_stat) {
    MutexLock lock(&mutex_);
    string output;
    for (auto& entry : is_stat ? exported_stats_ : exported_configs_) {
        CHECK_EQ(entry.first, entry.second->Name());
        output.append(StrCat(entry.first, "=", entry.second->GetValue(), "\n"));
    }
    return output;
}

string GlobalExporter::GetOne(const string& name, bool is_stat) {
    MutexLock lock(&mutex_);
    Exportee* e = FindWithDefault(
            is_stat ? exported_stats_ : exported_configs_, name, nullptr);
    return (e != nullptr) ? e->GetValue() : "";
}

string GlobalExporter::ResetStat(const string& name) {
    MutexLock lock(&mutex_);
    Exportee* e;
    if (!FindCopy(exported_stats_, name, &e))
        return StrCat("unknown stat: ", name, "\n");
    else if (e->ResetValue())
        return StrCat("reset ", name, ": done\n");
    else
        return StrCat("reset ", name, ": not supported\n");
}

string GlobalExporter::ResetAllStats() {
    MutexLock lock(&mutex_);
    string output;
    for (pair<const string, Exportee*>& p : exported_stats_) {
        CHECK_EQ(p.first, p.second->Name());
        if (p.second->ResetValue())
            output.append(StrCat("reset ", p.first, ": done\n"));
        else
            output.append(StrCat("reset ", p.first, ": not supported\n"));
    }
    return output;
}

string GlobalExporter::SetConfig(const string& name, const string& value) {
    MutexLock lock(&mutex_);
    Exportee* e;
    if (!FindCopy(exported_configs_, name, &e))
        return StrCat("unknown config: ", name, "\n");
    else if (e->SetValue(value))
        return StrCat("set ", name, " to ", e->GetValue(), "\n");
    else
        return StrCat("cannot set ", name, " to ", value, "\n");
}

}  // namespace cpp_base
