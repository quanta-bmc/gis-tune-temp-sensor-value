#include "sdbusplus.hpp"
#include <fstream>
#include <getopt.h>
#include <math.h>
#include <filesystem>
#include <unistd.h>
namespace fs = std::filesystem;

static std::string fan_sensor_path = "/xyz/openbmc_project/sensors/fan_tach/fan0";
static std::string temp_sensor_path = "/xyz/openbmc_project/sensors/temperature/inlet";
static std::string value_intf = "xyz.openbmc_project.Sensor.Value";
static std::string crit_alarm_intf = "xyz.openbmc_project.Sensor.Threshold.Critical";
static std::string warn_alarm_intf = "xyz.openbmc_project.Sensor.Threshold.Warning";
static std::string fan_dts_path = "/ahb/apb/i2c@83000/i2c-switch@75/i2c@2/fan_controller@2c";
static std::string temp_dts_path = "/ahb/apb/i2c@83000/i2c-switch@77/i2c@6/lm75@5c";
static std::string sensor_status = "";
static int crit_high_value = 35;
static int crit_low_value = 0;
static int fan_cnt = 6;
static double update_period = 0.1;
static bool verbose_flag = false;
static constexpr auto ofRoot = "/sys/firmware/devicetree/base";
static const auto emptyString = "";

static void usage(const char* name)
{
    fprintf(stderr,
            "Usage: %s [-v <verbose> -p <update-period> -s <temp-dbus-path> -f <fan-dbus-path> -d <temp-dts-path>  -t <fan-dts-path>\
             -h <crit-high-value> -l <crit-low-value>]\n"
            " -v, --verbose         Print the reading for every Input Sensor and the output for every Output Sensor every run\n"
            " -p, --update-period         Update inlet reading periods, default is 0.1 seconds\n"
            " -s, --temp-dbus-path         Indicate sensor to adjustment, default is /xyz/openbmc_project/sensors/temperature/inlet\n"
            " -f, --fan-dbus-path         Indicate fan sensor to reading, default is /xyz/openbmc_project/sensors/fan_tach/fan0\n"
            " -d, --temp-dts-path         Find sysfs sensor to reading, default is /ahb/apb/i2c@83000/i2c-switch@77/i2c@6/lm75@5c\n"
            " -t, --fan-dts-path         Find sysfs sensor to reading, default is /ahb/apb/i2c@83000/i2c-switch@75/i2c@2/fan_controller@2c\n"
            " -h, --crit-high-value         Setting temperature sensor upper critical threshold"
            " -l, --crit-low-value         Setting temperature sensor lower critical threshold",
            name);
}

static std::string findHwmonFromOFPath(std::string ofNode)
{
    static constexpr auto hwmonRoot = "/sys/class/hwmon";
    std::string sensor_path;

    auto fullOfPath = fs::path(ofRoot) / fs::path(ofNode).relative_path();

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

    for(int i=1; i<=fan_cnt; i++)
    {   
        ifile.open(hwmonfannum + "/fan" + std::to_string(i) + "_input");
        ifile >> fan_driver_val;
        rpm += fan_driver_val;
        ifile.close();
    }
    rpm /= 6;
    return rpm;
}

static double get_adjust_sensor_value(sdbusplus::bus::bus& bus, std::string service, double rpm, double temp)
{
    double adjust_value = 0;
    if (rpm >= 7300)
        adjust_value = round(temp/1000) - 1;
    else
        adjust_value = round(temp/1000) + 1;
    if (verbose_flag)
    {
        if (rpm >= 7300)
            std::cerr << "formula is Y-1"<< std::endl;
        else
            std::cerr << "formula is Y+1"<< std::endl;
        std::cerr << "Current rpm is : " << rpm << std::endl;
        std::cerr << "Inlet reading is : " << temp << std::endl;
        std::cerr << "Round inlet reading is : " << round(temp/1000) << std::endl;
        std::cerr << "Adjust temp is : " << adjust_value << std::endl;
        std::cerr << "Critical High threshold : " << crit_high_value << std::endl;
        std::cerr << "Critical Low threshold : " << crit_low_value << std::endl << std::endl;
    }
    return adjust_value;
}

static void check_sensor_threshold(sdbusplus::bus::bus& bus, std::string service, double value)
{
    if (value >= crit_high_value)
    {
        if(sensor_status != "CritHigh")
        {
            sensor_status = "CritHigh";
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            warn_alarm_intf, "WarningAlarmLow", false);
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            crit_alarm_intf, "CriticalAlarmLow", false);
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            warn_alarm_intf, "WarningAlarmHigh", true);
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            crit_alarm_intf, "CriticalAlarmHigh", true);
        }
    }
    else if (value < crit_high_value && value > crit_low_value)
    {
        if(sensor_status != "Normal")
        {
            sensor_status = "Normal";
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            warn_alarm_intf, "WarningAlarmLow", false);
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            crit_alarm_intf, "CriticalAlarmLow", false);
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            warn_alarm_intf, "WarningAlarmHigh", false);
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            crit_alarm_intf, "CriticalAlarmHigh", false);
        }
    }
    else
    {
        if(sensor_status != "CritLow")
        {
            sensor_status = "CritLow";
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            warn_alarm_intf, "WarningAlarmLow", true);
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            crit_alarm_intf, "CriticalAlarmLow", true);
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            warn_alarm_intf, "WarningAlarmHigh", false);
            util::SDBusPlus::setProperty(bus, service, temp_sensor_path,
                            crit_alarm_intf, "CriticalAlarmHigh", false);
        }
    }
}
