#!/bin/bash

## Initialize networking
#ip addr add 192.168.2.2/24 dev eth0
#ip addr add 192.168.2.3/24 dev eth1
#ip addr add 192.168.2.4/24 dev eth2

ip link add name bridge type bridge
ip link set bridge up
ip link set dev eth0 master bridge
ip link set dev eth1 master bridge
ip link set dev eth2 master bridge

ip addr add 192.168.2.2/24 dev bridge

## Conclude
cd
bash
