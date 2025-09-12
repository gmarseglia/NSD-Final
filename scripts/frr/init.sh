#!/bin/bash

# Turn on frr
ulimit -Sn 100000
/usr/lib/frr/watchfrr zebra bgpd ldpd &
sleep 3

# Execute local_init.sh if present
if [ -e /root/local_init.sh ]; then
    source /root/local_init.sh
fi
sleep 3

# Load frr.conf
if [ -e /root/frr.conf ]; then
    vtysh -f /root/frr.conf
fi

# # Oper terminal
cd
bash
