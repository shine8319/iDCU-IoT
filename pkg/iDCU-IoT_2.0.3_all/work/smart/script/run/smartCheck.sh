#!/bin/bash

redis="`pgrep redis-server | wc -l`"



if [ "$redis" -eq "0" ] ; then
echo "start redis-server"
sudo /work/smart/redis-server /work/config/redis.conf &
sleep 3
fi

redis="`pgrep redis-server | wc -l`"
if [ "$redis" -eq "1" ] ; then
	echo "already started redis-server"

	check="`pgrep SetupMana  | wc -l`"
        if [  "$check" -eq "0" ] ; then
        echo "start SetupManager"
        sudo /work/smart/SetupManager.o &
	sleep 1
        fi

	check2="`pgrep ModbusRT  | wc -l`"
        if [ "$check2" -eq "0" ] ; then
        echo "start ModbusRTU"
        sudo /work/smart/ModbusRTU.o &
	sleep 1
        fi

	check3="`pgrep SerialTo  | wc -l`"
        if [ "$check3" -eq "0" ] ; then
        echo "start SerialToEthernet"
        sudo /work/smart/SerialToEthernet.o &
	sleep 1
        fi

	check4="`pgrep DeviceSt  | wc -l`"
        if [ "$check4" -eq "0" ] ; then
        echo "start DeviceStatusCheck"
        sudo /work/smart/DeviceStatusCheck.o &
        fi


fi
