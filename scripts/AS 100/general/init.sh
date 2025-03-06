#!/bin/bash

ulimit -Sn 100000
/usr/lib/frr/watchfrr zebra bgpd ldpd &
sleep 1
vtysh < /root/commands.txt
cd
bash
