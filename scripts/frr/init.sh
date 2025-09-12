#!/bin/bash

# Turn on frr
ulimit -Sn 100000
/usr/lib/frr/watchfrr zebra bgpd ldpd &
sleep 1

# Execute local_init.sh if present
if [ -e /root/local_init.sh ]; then
    source /root/local_init.sh
fi

# Load frr.conf
vtysh -f /root/frr.conf

# Oper terminal
cd
bash