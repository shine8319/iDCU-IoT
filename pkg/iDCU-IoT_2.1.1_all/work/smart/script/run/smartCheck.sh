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

check="`pgrep USNBrid  | wc -l`"
if [  "$check" -eq "0" ] ; then
echo "start USNBridge"
sudo /work/smart/USNBridge.o &
sleep 1
fi
check="`pgrep USNMana  | wc -l`"
if [  "$check" -eq "0" ] ; then
echo "start USNManager"
sudo /work/smart/USNManager.o &
sleep 1
fi
check="`pgrep RealTimeD | wc -l`"
if [  "$check" -eq "0" ] ; then
echo "start RealTimeDataManager"
sudo /work/smart/RealTimeDataManager.o &
sleep 1
fi
check="`pgrep M2MMana | wc -l`"
if [  "$check" -eq "0" ] ; then
echo "start M2MManager"
sudo /work/smart/M2MManager.o &
sleep 1
fi
check="`pgrep DBSched | wc -l`"
if [  "$check" -eq "0" ] ; then
echo "start DBSchedule"
sudo /work/smart/DBSchedule.o &
sleep 1
fi




