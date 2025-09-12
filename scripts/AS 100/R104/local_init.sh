#!/bin/bash

## Pre MPLS setup
sysctl -w net.mpls.conf.lo.input=1
sysctl -w net.mpls.conf.eth1.input=1
sysctl -w net.mpls.conf.eth2.input=1
sysctl -w net.mpls.conf.eth3.input=1
sysctl -w net.mpls.platform_labels=100000
