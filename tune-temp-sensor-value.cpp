#include "tune-temp-sensor-value.hpp"

int main(int argc, char* argv[])
{
    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();
    std::string FanSensorService;
    std::string TempSensorService;
    std::string hwmon_fan_path;
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
                g_verbose_flag = true;
                break;
            case 'p':
                g_update_period = std::stod(optarg);
                std::cerr << "update-period : " << g_update_period << std::endl;
                break;
            case 's':
                g_temp_sensor_path = optarg;
                std::cerr << "temp-dbus-path : " << g_temp_sensor_path << std::endl;
                break;
            case 'f':
                g_fan_sensor_path = optarg;
                std::cerr << "fan-dbus-path : " << g_fan_sensor_path << std::endl;
                break;
            case 'd':
                g_temp_dts_path = optarg;
                std::cerr << "temp-dts-path : " << g_temp_dts_path << std::endl;
                break;
            case 't':
                g_fan_dts_path = optarg;
                std::cerr << "fan-dts-path " << g_fan_dts_path << std::endl;
                break;
            case 'h':
                g_crit_high_value = std::stod(optarg);
                std::cerr << "crit-high-value " << g_crit_high_value << std::endl;
                break;
            case 'l':
                g_crit_low_value = std::stod(optarg);
                std::cerr << "crit-low-value " << g_crit_low_value << std::endl;
                break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
        std::cerr << std::endl;
    }

    TempSensorService = util::SDBusPlus::getService(bus, g_value_intf, g_temp_sensor_path);

    hwmon_fan_path = find_hwmon_from_OFPath(g_fan_dts_path);
    std::ifstream ifile(find_hwmon_from_OFPath(g_temp_dts_path)+"/temp1_input");

    if(ifile.is_open()){
        while(true)
        {
            sleep(g_update_period);
            rpm=get_ave_rpm(hwmon_fan_path);
            if (rpm == -1)
            {
                continue;
            }
            ifile.clear();
            ifile.seekg(0);
            ifile >> driver_val;
            adjust_sensor_val = get_adjust_sensor_value(bus, rpm, driver_val);
            util::SDBusPlus::setProperty(bus, TempSensorService, g_temp_sensor_path,
                                        g_value_intf, "Value", adjust_sensor_val);
            check_sensor_threshold(bus, TempSensorService, adjust_sensor_val);
        }
        ifile.close();
    }
    else
    {
        std::cerr << "Inlet Temperature Sensor Does Not to Open !!!" << std::endl;
        ifile.close();
        return -1;
    }
    
    return 0;
}
