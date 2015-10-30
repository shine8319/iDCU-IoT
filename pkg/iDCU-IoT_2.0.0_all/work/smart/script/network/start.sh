#!/bin/bash

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
/work/script/network/lan/eth0.sh
	fi
fi

if [ $index == "wlan" ]
then
	if [ $chr == "1" ]
	then
#echo index:$index
#echo chr:$chr
/work/script/network/wlan/wlan0.sh
	fi
fi


done  < /work/config/networkUse.config
