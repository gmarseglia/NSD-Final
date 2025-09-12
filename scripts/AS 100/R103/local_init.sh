#!/bin/bash

## Create the VRF
ip link add vpn10 type vrf table 10
ip link set vpn10 up
ip link set eth0 master vpn10

## Pre MPLS setup
sysctl -w net.mpls.conf.lo.input=1
sysctl -w net.mpls.conf.eth3.input=1
sysctl -w net.mpls.conf.vpn10.input=1
sysctl -w net.mpls.platform_labels=100000
