#!/bin/bash
for f in /dev/ttyUSB?

do
#check="`udevadm info -q property -n $f | grep ID_VENDOR_ID | grep 10c4 | wc -l`"
check="`udevadm info -q property -n $f | grep ID_VENDOR_ID | grep 0403 | wc -l`"

if [ "$check" -eq "1" ] ; then
sudo rm -rf /work/config/devName
sudo echo $f >> /work/config/devName
fi

done
