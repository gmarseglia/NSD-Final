#!/bin/bash

OPENVPN="/root/openvpn"

## Initialize networking
ip addr add 10.1.3.2/30 dev eth1
ip addr add 192.168.3.1/24 dev eth0
ip route add default via 10.1.3.1

sysctl -w net.ipv4.ip_forward=1 >/dev/null 2>&1

## Configure OpenVPN ccd
mkdir -p ${OPENVPN}/ccd
echo "iroute 192.168.1.0 255.255.255.0" > ${OPENVPN}/ccd/client1
echo "iroute 192.168.2.0 255.255.255.0" > ${OPENVPN}/ccd/client2

## Conclude
cd
exec bash