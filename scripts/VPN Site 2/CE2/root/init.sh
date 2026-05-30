#!/bin/bash

## Initialize networking
ip link add link eth0 name eth0.32 type vlan id 32
ip link add link eth0 name eth0.95 type vlan id 95
ip link set eth0.32 up
ip link set eth0.95 up

ip addr add 10.1.2.2/30 dev eth1
ip addr add 192.168.2.1/24 dev eth0
ip addr add 192.168.32.1/24 dev eth0.32
ip addr add 192.168.95.1/24 dev eth0.95

ip route add default via 10.1.2.1

echo 1 > /proc/sys/net/ipv4/ip_forward

openvpn --config /root/client2.conf --daemon

## Conclude
cd
exec bash