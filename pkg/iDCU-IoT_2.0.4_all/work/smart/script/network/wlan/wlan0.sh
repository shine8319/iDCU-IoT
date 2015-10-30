#!/bin/bash

check="`pgrep wpa_supp`"
kill  $check
sleep 1
ifconfig wlan0 up
sleep 1

sudo wpa_supplicant -iwlan0 -c /work/script/network/wlan/security/wpa_supplicant.conf &
sleep 5


while read line
do

index=$(echo $line | awk '{print $1}')
chr=$(echo $line | awk '{print $2}')

if [ $index == "address" ]
then
ifconfig wlan0 $chr
else if [ $index == "gateway" ]
then
route add default gw $chr wlan0
else if [ $index == "netmask" ]
then
ifconfig wlan0 $index $chr
else
echo nameserver $chr > /etc/resolv.conf
fi
fi
fi

echo index:$index
echo chr:$chr

done  < /work/config/wlan0.config
