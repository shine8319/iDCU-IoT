#!/bin/bash

lanEnable="0"
wlanEnable="0"

while read line
do

index=$(echo $line | awk '{print $1}')
chr=$(echo $line | awk '{print $2}')

if [ $index == "lan" ]
then
	if [ $chr == "1" ]
	then
#echo index:$index
#echo chr:$chr
#/work/script/network/lan/eth0.sh
	lanEnable="1"
	fi
fi

if [ $index == "wlan" ]
then
	if [ $chr == "1" ]
	then
#echo index:$index
#echo chr:$chr
#/work/script/network/wlan/wlan0.sh
	wlanEnable="1"
	fi
fi
done  < /work/config/networkUse.config


while read line
do

index=$(echo $line | awk '{print $1}')
chr=$(echo $line | awk '{print $2}')

if [ $lanEnable == "1" ]
then
    if [ $index == "lanauto" ]
    then
	    if [ $chr == "1" ]
	    then
		/work/script/network/lan/eth0Static.sh
	    else
		/work/script/network/lan/eth0.sh
	    fi
    fi
fi

if [ $wlanEnable == "1" ]
then
    if [ $index == "wlanauto" ]
    then
	    if [ $chr == "1" ]
	    then
		/work/script/network/wlan/wlan0Static.sh
	    else
		/work/script/network/wlan/wlan0.sh
	    fi
    fi
fi
done  < /work/config/options.config

/work/script/searchBridge.sh
/work/script/run/smartCheck.sh
