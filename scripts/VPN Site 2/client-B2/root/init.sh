#!/bin/bash

ip addr add  192.168.95.1/24 dev eth0
ip route add default via 192.168.95.1

cd
exec bash

# wpa_supplicant -c /etc/wpa_supplicant.conf -D wired -i eth0