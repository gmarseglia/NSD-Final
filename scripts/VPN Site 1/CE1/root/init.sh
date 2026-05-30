#!/bin/bash

## Initialize networking
ip addr add 10.1.1.2/30 dev eth1
ip addr add 192.168.1.1/24 dev eth0
ip route add default via 10.1.1.1

echo 1 > /proc/sys/net/ipv4/ip_forward

openvpn --config /root/client1.conf --daemon

## Conclude
cd
exec bash