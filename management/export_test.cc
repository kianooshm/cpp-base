#include <gtest/gtest.h>
#include <string>
#include "cpp-base/integral_types.h"
#include "cpp-base/management/exportee.h"
#include "cpp-base/management/exported_config.h"
#include "cpp-base/management/exported_stat.h"
#include "cpp-base/management/global_exporter.h"
#include "cpp-base/string/join.h"

using cpp_base::ExportedStatCallback;
using cpp_base::ExportedStatVariable;
using cpp_base::ExportedConfigCallback;
using cpp_base::ExportedConfigParameter;
using cpp_base::StrCat;
using cpp_base::GlobalExporter;
using std::string;

class ExportedVarTest : public ::testing::Test {};

TEST_F(ExportedVarTest, ExportedLegacyStatVariableTest) {
    cpp_base::Mutex mutex;
    int var = 10;
    ExportedStatVariable<int> exp_var("my_var", &var, true, &mutex);

    // Test initial value:
    EXPECT_EQ("my_var", exp_var.Name());
    EXPECT_EQ("10", exp_var.GetValue());

    // Change value:
    var = 20;
    EXPECT_EQ("20", exp_var.GetValue());

    // Reset:
    EXPECT_TRUE(exp_var.ResetValue());
    EXPECT_EQ(0, var);
    EXPECT_EQ("0", exp_var.GetValue());

    // Try a non-resetable one:
    var = 50;
    ExportedStatVariable<int> exp_var2("my_var2", &var, false, &mutex);
    EXPECT_FALSE(exp_var2.ResetValue());
    // Shouldn't have been reset:
    EXPECT_EQ(50, var);
    EXPECT_EQ("50", exp_var2.GetValue());
}

TEST_F(ExportedVarTest, ExportedStatVariableTest) {
    // Try with a std::atomic<> type.
    std::atomic<int> num(100);
    ExportedStatVariable<std::atomic<int>> exportee("my_exp", &num, true);
    // Test initial value:
    EXPECT_EQ("100", exportee.GetValue());

    // Reset: (Note: there is no Set(value) for stats; it's for configs)
    EXPECT_TRUE(exportee.ResetValue());
    // Must have become 0:
    EXPECT_EQ(0, num.load());
    EXPECT_EQ("0", exportee.GetValue());

    // Non-resetable:
    num.store(200);
    ExportedStatVariable<std::atomic<int>> exportee2("my_exp2", &num, false);
    EXPECT_FALSE(exportee2.ResetValue());
    // Must still be 200, not 0:
    EXPECT_EQ(200, num.load());
    EXPECT_EQ("200", exportee2.GetValue());
}

TEST_F(ExportedVarTest, ExportedStatCallbackTest) {
    int num1 = 10;
    ExportedStatCallback<int> exp_cb1(
            "num1",
            std::function<int()>( [&]() {return num1;} ));

    int num2 = 20;
    ExportedStatCallback<int> exp_cb2(
            "num2",
            std::function<int()>( [&]() {return num2;} ),
            std::function<void()>( [&]() {num2 = 0;} ));

    // Test initial values:
    EXPECT_EQ("num1", exp_cb1.Name());
    EXPECT_EQ("num2", exp_cb2.Name());
    EXPECT_EQ("10", exp_cb1.GetValue());
    EXPECT_EQ("20", exp_cb2.GetValue());

    // Test updating the values:
    num1 += 5;
    num2 += 5;
    EXPECT_EQ("15", exp_cb1.GetValue());
    EXPECT_EQ("25", exp_cb2.GetValue());

    // Test resetting the values:
    EXPECT_FALSE(exp_cb1.ResetValue());  // exp_cb1 is non-resettable
    EXPECT_TRUE(exp_cb2.ResetValue());   // exp_cb1 is resettable
    // Variables should work as they did (not frozen) after reset:
    num1 += 2;
    num2 += 2;
    EXPECT_EQ("17", exp_cb1.GetValue());
    EXPECT_EQ("2", exp_cb2.GetValue());
}

TEST_F(ExportedVarTest, ExportedConfigParameterTest) {
    // Primitive-variable config params are quite similar to primitive-variable
    // stats, except that they support Set(value) and do not support Reset().
    std::atomic<int> num(100);
    ExportedConfigParameter<std::atomic<int>> exportee(
        "my_num",
        &num,
        std::function<bool(int)>( [](int x) {return (x < 1000); } ));

    // Initial value:
    EXPECT_EQ("100", exportee.GetValue());

    // Set a value:
    EXPECT_TRUE(exportee.SetValue("200"));
    EXPECT_EQ(200, num.load());
    EXPECT_EQ("200", exportee.GetValue());

    // Try to set a value that fails the validation check (being under 1000):
    EXPECT_FALSE(exportee.SetValue("1001"));
    EXPECT_EQ(200, num.load());
    EXPECT_EQ("200", exportee.GetValue());

    // Try to set an invalid (non-number) string:
    EXPECT_FALSE(exportee.SetValue("300xxx"));
    EXPECT_EQ(200, num.load());
    EXPECT_EQ("200", exportee.GetValue());
}

// Besides lambda (above), we'd also like to try class-based config functions.
template <class T>
class TestParam {
  public:
    explicit TestParam(T value) : value_(value) {}
    TestParam() : value_() {}
    T Get() const { return value_; }
    bool Set(T value) {
        value_ = value;
        return true;
    }

  private:
    T value_;
};

TEST_F(ExportedVarTest, ExportedConfigCallBackTest) {
    using namespace std::placeholders;

    TestParam<int> my_int(10);
    ExportedConfigCallback<int> exported_int(
            "my_int",
            std::bind<int>(&TestParam<int>::Get, &my_int),
            std::bind<bool>(&TestParam<int>::Set, &my_int, _1));

    TestParam<float> my_float(10.5);
    ExportedConfigCallback<float> exported_float(
            "my_float",
            std::bind<float>(&TestParam<float>::Get, &my_float),
            std::bind<bool>(&TestParam<float>::Set, &my_float, _1));

    // Test initial values:
    EXPECT_EQ("10", exported_int.GetValue());
    EXPECT_EQ("10.5", exported_float.GetValue());

    // Set the values via the exporter:
    EXPECT_TRUE(exported_int.SetValue("20"));
    EXPECT_EQ(20, my_int.Get());
    EXPECT_EQ("20", exported_int.GetValue());
    EXPECT_TRUE(exported_float.SetValue("20.5"));
    EXPECT_EQ(20.5, my_float.Get());
    EXPECT_EQ("20.5", exported_float.GetValue());

    // Set to invalid value:
    EXPECT_FALSE(exported_float.SetValue("30xx.0"));
    EXPECT_EQ(20.5, my_float.Get());
    EXPECT_EQ("20.5", exported_float.GetValue());
}

namespace {
// Checks whether the given exported stat/config name exists once and only once,
// and it has the given value. 'is_stat' is to distinguish stats from configs.
void RenderCheck(const string& name, const string& value,
                 bool is_stat, int line_number) {
    GlobalExporter* g_exporter = GlobalExporter::Instance();
    string s, expected = StrCat(name, "=", value, "\n");

    s = is_stat ? g_exporter->RenderStat(name) : g_exporter->RenderConfig(name);
    EXPECT_EQ(expected, s) << line_number;

    s = is_stat ? g_exporter->RenderAllStats() : g_exporter->RenderAllConfigs();
    EXPECT_NE(string::npos, s.find(expected)) << s << " ; " << line_number;

    // Make sure only once line exists:
    int i = s.find(StrCat(name, "="));
    EXPECT_NE(string::npos, i) << s << " ; " << line_number;
    i = s.find(StrCat(name, "="), i + 1);
    EXPECT_EQ(string::npos, i) << s << " ; " << line_number;
}
}

TEST_F(ExportedVarTest, ExportTest) {
    // Non-existing variable name:
    string s = GlobalExporter::Instance()->RenderStat("xxx");
    EXPECT_EQ(0, s.find("unknown stat")) << s;

#define STAT_RENDER_CHECK(name, value) RenderCheck(name, value, true, __LINE__)
#define CONF_RENDER_CHECK(name, value) RenderCheck(name, value, false, __LINE__)

    // Case 1: a legacy, primitive-variable stat, protected by a mutex:
    cpp_base::Mutex mutex;
    int64 var = 10;
    ExportedStatVariable<int64>* exp_var =
            new ExportedStatVariable<int64>("my_var", &var, true, &mutex);
    STAT_RENDER_CHECK("my_var", "10");

    // Test updating the value:
    var += 5;
    STAT_RENDER_CHECK("my_var", "15");

    // Test reseting the value:
    s = GlobalExporter::Instance()->ResetStat("my_var");
    EXPECT_NE(string::npos, s.find("done")) << s;
    EXPECT_EQ(0, var);

    // This should remove the variable from the global list.
    delete exp_var;
    s = GlobalExporter::Instance()->RenderStat("my_var");
    EXPECT_EQ(0, s.find("unknown stat")) << s;
    s = GlobalExporter::Instance()->RenderAllStats();
    EXPECT_EQ(string::npos, s.find("my_var=")) << s;

    // Empty set, but a reset-all shouldn't break anything.
    GlobalExporter::Instance()->ResetAllStats();

    // Case 2: try with an exported stat callback:
    int var2 = 20;
    ExportedStatCallback<int> exp_cb(
            "my_var2",
            std::function<int()>( [&]() {return var2;} ),
            std::function<void()>( [&]() {var2 = 0;} ));
    // Should render correctly:
    STAT_RENDER_CHECK("my_var2", "20");
    // Reset:
    GlobalExporter::Instance()->ResetAllStats();
    STAT_RENDER_CHECK("my_var2", "0");

    // Case 3: an exported config callback:
    using namespace std::placeholders;
    int num = 10;
    ExportedConfigCallback<int>* exported_int = new ExportedConfigCallback<int>(
            "my_conf",
            std::function<int()>( [&]() {return num;} ),
            std::function<bool(int)>( [&](int x) {num = x; return true;} ));

    // There should exist the config param we just added.
    CONF_RENDER_CHECK("my_conf", "10");

    // Update the value:
    s = GlobalExporter::Instance()->SetConfig("my_conf", "20");
    EXPECT_EQ(0, s.find("set my_conf to 20"));
    CONF_RENDER_CHECK("my_conf", "20");

    // Try setting an invalid value:
    s = GlobalExporter::Instance()->SetConfig("my_conf", "30xx");
    EXPECT_EQ(0, s.find("cannot set my_conf"));
    CONF_RENDER_CHECK("my_conf", "20");

    // Try updating a non-existing config param:
    s = GlobalExporter::Instance()->SetConfig("xxx", "1");
    EXPECT_EQ(0, s.find("unknown "));

    // This should 'unexport' the config param.
    delete exported_int;
    s = GlobalExporter::Instance()->RenderConfig("my_conf");
    EXPECT_EQ(string::npos, s.find("my_conf=20"));
    s = GlobalExporter::Instance()->RenderAllConfigs();
    EXPECT_EQ(string::npos, s.find("my_conf=20"));
}
