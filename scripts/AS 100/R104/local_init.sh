#!/bin/bash

## Create the VRF
ip link add vpn10 type vrf table 10
ip link set vpn10 up
ip link set eth0 master vpn10

## Pre MPLS setup
echo 'net.mpls.conf.lo.input = 1' >> /etc/sysctl.conf
echo 'net.mpls.conf.eth1.input = 1' >> /etc/sysctl.conf
echo 'net.mpls.conf.eth2.input = 1' >> /etc/sysctl.conf
echo 'net.mpls.conf.eth3.input = 1' >> /etc/sysctl.conf
echo 'net.mpls.platform_labels = 100000' >> /etc/sysctl.conf

sysctl -p
