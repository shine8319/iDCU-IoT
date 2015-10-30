#!/bin/bash

killall SetupManager.o
killall ModbusRTU.o
killall SerialToEthernet.o
killall DeviceStatusCheck.o

killall USNBridge.o
killall USNManager.o
killall RealTimeDataManager.o
killall M2MManager.o
killall DBSchedule.o

echo "restart Programs"
/work/script/run/smartCheck.sh


