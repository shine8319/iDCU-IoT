#!/bin/bash

redis="`pgrep redis-server | wc -l`"
check="`pgrep SetupMana  | wc -l`"
check2="`pgrep ModbusRT  | wc -l`"
check3="`pgrep SerialTo  | wc -l`"
check4="`pgrep DeviceSt  | wc -l`"



if [ "$redis" -eq "0" ] ; then
echo "start redis-server"
sudo /work/smart/redis-server /work/config/redis.conf &
sleep 3
fi

redis="`pgrep redis-server | wc -l`"
if [ "$redis" -eq "1" ] ; then
	echo "already started redis-server"

        if [  "$check" -eq "0" ] ; then
        echo "start SetupManager"
        sudo /work/smart/SetupManager.o &
	sleep 1
        fi

        if [ "$check2" -eq "0" ] ; then
        echo "start ModbusRTU"
        sudo /work/smart/ModbusRTU.o &
	sleep 1
        fi

        if [ "$check3" -eq "0" ] ; then
        echo "start SerialToEthernet"
        sudo /work/smart/SerialToEthernet.o &
	sleep 1
        fi

        if [ "$check4" -eq "0" ] ; then
        echo "start DeviceStatusCheck"
        sudo /work/smart/DeviceStatusCheck.o &
        fi


fi
