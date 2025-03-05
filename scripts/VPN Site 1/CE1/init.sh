#!/bin/bash

## Initialize networking
ip addr add 192.168.1.1/24 dev eth1
ip addr add 10.1.1.2/30 dev eth0
ip route add default via 10.1.1.1

## Conclude
cd
bash
