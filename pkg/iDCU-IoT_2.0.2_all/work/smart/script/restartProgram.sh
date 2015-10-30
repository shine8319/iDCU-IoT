#!/bin/bash

killall SetupManager.o &
killall ModbusRTU.o &
killall SerialToEthernet.o &
killall DeviceStatusCheck.o &


echo "restart Programs"
/work/script/run/smartCheck.sh &


