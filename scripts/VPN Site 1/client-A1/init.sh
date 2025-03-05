#!/bin/bash

## Initialize networking
ip addr add 192.168.1.2/24 dev eth0
ip route add default via 192.168.1.1

## Conclude
cd
bash