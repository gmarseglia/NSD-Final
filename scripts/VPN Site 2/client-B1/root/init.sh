#!/bin/bash

ip addr add  192.168.32.2/24 dev eth0
ip route add default via 192.168.32.1 dev eth0

cd
exec bash

# wpa_supplicant -c /etc/wpa_supplicant.conf -D wired -i eth0 -B
# wpa_cli -i eth0 logoff
# wpa_cli -i eth0 logon