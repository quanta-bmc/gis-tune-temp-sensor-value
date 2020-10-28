#include "tune-temp-sensor-value.hpp"

int main(int argc, char* argv[])
{
    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();
    std::string FanSensorService;
    std::string TempSensorService;
    std::string hwmon_fan_num;
    int opt;
    double rpm;
    double driver_val;
    double adjust_sensor_val;

    static const struct option long_options[] = 
    {
        {"verbose", 0, NULL, 'v'},
        {"update-period", 1, NULL, 'p'},
        {"temp-dbus-path", 1, NULL, 's'},
        {"fan-dbus-path", 1, NULL, 'f'},
        {"temp-dts-path", 1, NULL, 'd'},
        {"fan-dts-path", 1, NULL, 't'},
        {"crit-high-value", 1, NULL, 'h'},
        {"crit-low-value", 1, NULL, 'l'},
        {0,0,0,0}
    };

    while ((opt = getopt_long(argc, argv, "vp:s:f:d:t:h:l:", long_options, NULL)) != -1)
    {
        switch (opt)
        {
            case 'v':
                verbose_flag = true;
                break;
            case 'p':
                update_period = std::stod(optarg);
                std::cerr << "update-period : " << update_period << std::endl;
                break;
            case 's':
                temp_sensor_path = optarg;
                std::cerr << "temp-dbus-path : " << temp_sensor_path << std::endl;
                break;
            case 'f':
                fan_sensor_path = optarg;
                std::cerr << "fan-dbus-path : " << fan_sensor_path << std::endl;
                break;
            case 'd':
                temp_dts_path = optarg;
                std::cerr << "temp-dts-path : " << temp_dts_path << std::endl;
                break;
            case 't':
                fan_dts_path = optarg;
                std::cerr << "fan-dts-path " << fan_dts_path << std::endl;
                break;
            case 'h':
                crit_high_value = std::stod(optarg);
                std::cerr << "crit-high-value " << crit_high_value << std::endl;
                break;
            case 'l':
                crit_low_value = std::stod(optarg);
                std::cerr << "crit-low-value " << crit_low_value << std::endl;
                break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
        std::cerr << std::endl;
    }

    while(FanSensorService.empty() || TempSensorService.empty())
    {
        FanSensorService = util::SDBusPlus::getService(bus, value_intf, fan_sensor_path);
        TempSensorService = util::SDBusPlus::getService(bus, value_intf, temp_sensor_path);
    }

    hwmon_fan_num = findHwmonFromOFPath(fan_dts_path);
    std::ifstream ifile(findHwmonFromOFPath(temp_dts_path)+"/temp1_input");

    while(true)
    {
        rpm=get_ave_rpm(hwmon_fan_num);
        ifile >> driver_val;
        adjust_sensor_val = get_adjust_sensor_value(bus, TempSensorService, rpm, driver_val);
        util::SDBusPlus::setProperty(bus, TempSensorService, temp_sensor_path,
                                    value_intf, "Value", adjust_sensor_val);
        check_sensor_threshold(bus, TempSensorService, adjust_sensor_val);
        sleep(update_period);
    }

    ifile.close();
    return 0;
}
