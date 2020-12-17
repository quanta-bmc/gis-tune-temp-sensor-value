#include "sdbusplus.hpp"
#include <fstream>
#include <getopt.h>
#include <math.h>
#include <filesystem>
#include <unistd.h>

enum class threshold_state
{
  Init,
  Normal,
  CritHigh,
  CritLow
};

static std::string g_fan_sensor_path = "/xyz/openbmc_project/sensors/fan_tach/fan0";
static std::string g_temp_sensor_path = "/xyz/openbmc_project/sensors/temperature/inlet";
static std::string g_value_intf = "xyz.openbmc_project.Sensor.Value";
static std::string g_crit_alarm_intf = "xyz.openbmc_project.Sensor.Threshold.Critical";
static std::string g_warn_alarm_intf = "xyz.openbmc_project.Sensor.Threshold.Warning";
static std::string g_fan_dts_path = "/ahb/apb/i2c@83000/i2c-switch@75/i2c@2/fan_controller@2c";
static std::string g_temp_dts_path = "/ahb/apb/i2c@83000/i2c-switch@77/i2c@6/lm75@5c";
static int g_crit_high_value = 35;
static int g_crit_low_value = 0;
static double g_update_period = 0.1;
static bool g_verbose_flag = false;
static threshold_state g_sensor_status;
static constexpr int g_fan_cnt = 6;

namespace fs = std::filesystem;

static void usage(const char* name)
{
    std::cerr << "Usage : " 
            <<"[-v <verbose> -p <update-period> -s <temp-dbus-path> -f <fan-dbus-path> "
            <<"-d <temp-dts-path> -t <fan-dts-path> -h <crit-high-value> -l <crit-low-value>]\n"
            <<"-v, --verbose\
            Print the reading for every Input Sensor and the output for every Output Sensor every run\n"
            <<"-p, --update-period\
            Update inlet reading periods, default is 0.1 seconds\n"
            <<"-s, --temp-dbus-path\
            Indicate sensor to adjustment, default is /xyz/openbmc_project/sensors/temperature/inlet\n"
            <<"-f, --fan-dbus-path\
            Indicate fan sensor to reading, default is /xyz/openbmc_project/sensors/fan_tach/fan0\n"
            <<"-d, --temp-dts-path\
            Find sysfs sensor to reading, default is /ahb/apb/i2c@83000/i2c-switch@77/i2c@6/lm75@5c\n"
            <<"-t, --fan-dts-path\
            Find sysfs sensor to reading, default is /ahb/apb/i2c@83000/i2c-switch@75/i2c@2/fan_controller@2c\n"
            <<"-h, --crit-high-value\
            Setting temperature sensor upper critical threshold\n"
            <<"-l, --crit-low-value\
            Setting temperature sensor lower critical threshold"
            << std::endl;
}

static std::string find_hwmon_from_OFPath(std::string ofNode)
{
    static constexpr auto hwmonRoot = "/sys/class/hwmon";
    static constexpr auto ofRoot = "/sys/firmware/devicetree/base";
    static constexpr auto emptyString = "";
    std::string sensor_path;
    std::string fullOfPath = fs::path(ofRoot) / fs::path(ofNode).relative_path();

    for (const auto& hwmonInst : fs::directory_iterator(hwmonRoot))
    {
        auto path = hwmonInst.path();
        path /= "of_node";

        try
        {
            path = fs::canonical(path);
        }
        catch (const std::system_error& e)
        {
            // realpath may encounter ENOENT (Hwmon
            // instances have a nasty habit of
            // going away without warning).
            continue;
        }

        if (path == fullOfPath)
        {
            sensor_path = hwmonInst.path();
            return sensor_path;
        }
    }

    return emptyString;
}

static double get_ave_rpm(std::string hwmonfannum)
{
    double rpm = 0;
    double fan_driver_val = 0;
    std::ifstream  ifile;

    for(int i=1; i<=g_fan_cnt; i++)
    {
        ifile.open(hwmonfannum + "/fan" + std::to_string(i) + "_input");
        if(ifile.is_open())
        {
            ifile >> fan_driver_val;
            rpm += fan_driver_val;
            ifile.close();
        }
    }
    rpm /= g_fan_cnt;
    return rpm;
}

static double get_adjust_sensor_value(sdbusplus::bus::bus& bus, double rpm, double temp)
{
    double adjust_value = 0;

    if (rpm >= 7300)
        adjust_value = round(temp/1000) - 1;
    else
        adjust_value = round(temp/1000) + 1;
    if (g_verbose_flag)
    {
        if (rpm >= 7300)
            std::cerr << "formula is Y-1"<< std::endl;
        else
            std::cerr << "formula is Y+1"<< std::endl;
        std::cerr << "Current rpm is : " << rpm << std::endl;
        std::cerr << "Inlet reading is : " << temp << std::endl;
        std::cerr << "Round inlet reading is : " << round(temp/1000) << std::endl;
        std::cerr << "Adjust temp is : " << adjust_value << std::endl;
        std::cerr << "Critical High threshold : " << g_crit_high_value << std::endl;
        std::cerr << "Critical Low threshold : " << g_crit_low_value << std::endl << std::endl;
    }
    return adjust_value;
}

static void check_sensor_threshold(sdbusplus::bus::bus& bus, std::string service, double value)
{
    threshold_state new_sensor_status;
    bool warn_low;
    bool crit_low;
    bool warn_high;
    bool crit_high;

    if (value >= g_crit_high_value)
    {
        new_sensor_status = threshold_state::CritHigh;
        warn_low = false;
        crit_low = false;
        warn_high = true;
        crit_high = true;
    }
    else if (value < g_crit_high_value && value > g_crit_low_value)
    {
        new_sensor_status = threshold_state::Normal;
        warn_low = false;
        crit_low = false;
        warn_high = false;
        crit_high = false;
    }
    else
    {
        new_sensor_status = threshold_state::CritLow;
        warn_low = true;
        crit_low = true;
        warn_high = false;
        crit_high = false;
    }

    if (new_sensor_status != g_sensor_status)
    {
        g_sensor_status = new_sensor_status;
        util::SDBusPlus::setProperty(bus, service, g_temp_sensor_path,
                        g_warn_alarm_intf, "WarningAlarmLow", warn_low);
        util::SDBusPlus::setProperty(bus, service, g_temp_sensor_path,
                        g_crit_alarm_intf, "CriticalAlarmLow", crit_low);
        util::SDBusPlus::setProperty(bus, service, g_temp_sensor_path,
                        g_warn_alarm_intf, "WarningAlarmHigh", warn_high);
        util::SDBusPlus::setProperty(bus, service, g_temp_sensor_path,
                        g_crit_alarm_intf, "CriticalAlarmHigh", crit_high);
    }
}
