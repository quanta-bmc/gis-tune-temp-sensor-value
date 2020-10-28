# gis-tune-temp-sensor-value

1.This demand would update inlet sensor value
to dbus every 0.1 seconds with Quanta thermal team
adjusting formula.

2.Execute with parameter :
-v, --verbose : Print the reading for every Input Sensor
and the output for every Output Sensor every run

-p, --update-period : Update inlet reading periods,
default is 0.1 seconds

-s, --temp-dbus-path : Indicate sensor to adjustment,
default is /xyz/openbmc_project/sensors/temperature/inlet

-f, --fan-dbus-path : Indicate fan sensor to reading,
default is /xyz/openbmc_project/sensors/fan_tach/fan0

-d, --temp-dts-path : Find sysfs sensor to reading,
default is /ahb/apb/i2c@83000/i2c-switch@77/i2c@6/lm75@5c

-t, --fan-dts-path : Find sysfs sensor to reading,
default is /ahb/apb/i2c@83000/i2c-switch@75/i2c@2/fan_controller@2c

-h, --crit-high-value :
Setting temperature sensor upper critical threshold

-l, --crit-low-value :
Setting temperature sensor lower critical threshold

3.Setting hwmon/ahb/apb/i2c@83000/i2c-switch@77/i2c@6/lm75@5c.conf
to update inlet sensor value to dbus once a day, so that this script
would adjust inlet sensor value to dbus every $1 seconds.

4.On our gis board test, the cpu usage of this daemon is between 0
and 1.