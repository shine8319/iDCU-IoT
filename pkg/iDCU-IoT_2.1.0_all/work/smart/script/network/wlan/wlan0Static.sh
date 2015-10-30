#!/bin/bash

check="`pgrep wpa_supp`"
kill  $check
sleep 1
ifconfig wlan0 up
sleep 1

sudo wpa_supplicant -iwlan0 -c /work/script/network/wlan/security/wpa_supplicant.conf &
sleep 5

dhclient wlan0
