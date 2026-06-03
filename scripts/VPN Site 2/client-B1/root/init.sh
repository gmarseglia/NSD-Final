#!/bin/bash

ip addr add  192.168.32.2/24 dev eth0
ip route add default via 192.168.32.1 dev eth0

# Install the packages
dpkg -i /root/packages/libcurl4_7.81.0-1ubuntu1.24_amd64.deb
dpkg -i /root/packages/curl_7.81.0-1ubuntu1.24_amd64.deb

wpa_supplicant -c /etc/wpa_supplicant.conf -D wired -i eth0 -B

cd
exec bash

# wpa_cli -i eth0 logoff
# wpa_cli -i eth0 logon