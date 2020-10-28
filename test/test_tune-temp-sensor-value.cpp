#include  <gtest/gtest.h>
#include "../tune-temp-sensor-value.hpp"

TEST (rpmHigh, get_adjust_sensor_value) {
    sdbusplus::bus::bus bus=sdbusplus::bus::new_default();
    double rpm = 16661;
    double temp = 25347;
    double adjust_value = 24; // expected Adjust temp
    std::string service = "Test service";
    EXPECT_EQ(adjust_value,get_adjust_sensor_value(bus,service,rpm,temp));
}

TEST (rpmLow, get_adjust_sensor_value) {
    sdbusplus::bus::bus bus=sdbusplus::bus::new_default();
    double rpm = 480;
    double temp = 41000;
    double adjust_value = 42; // expected Adjust temp
    std::string service = "Test service";
    EXPECT_EQ(adjust_value,get_adjust_sensor_value(bus,service,rpm,temp));
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}
