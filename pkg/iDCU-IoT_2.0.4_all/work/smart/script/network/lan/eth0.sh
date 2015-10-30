#!/bin/bash

#wpa_supplicant -iwlan0 -c /work/config/wpa_supplicant.conf &
#sleep 3


while read line
do

index=$(echo $line | awk '{print $1}')
chr=$(echo $line | awk '{print $2}')

if [ $index == "address" ]
then
ifconfig eth0 $chr
else if [ $index == "gateway" ]
then
route add default gw $chr eth0
else if [ $index == "netmask" ]
then
ifconfig eth0 $index $chr
else
echo nameserver $chr > /etc/resolv.conf
fi
fi
fi

echo index:$index
echo chr:$chr

done  < /work/config/eth0.config
