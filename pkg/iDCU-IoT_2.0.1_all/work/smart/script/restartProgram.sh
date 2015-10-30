#!/bin/bash

killall SetupManager.o &
killall ModbusRTU.o &
killall SerialToEthernet.o &


echo "restart Programs"
/work/script/run/smartCheck.sh &


