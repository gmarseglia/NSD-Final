#!/bin/bash

ulimit -Sn 100000
/usr/lib/frr/watchfrr zebra bgpd ldpd &
sleep 1
if [ -e /root/local_init.sh ]; then
    source /root/local_init.sh
fi
vtysh < /root/commands.txt
cd
bash
