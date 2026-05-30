#!/bin/bash

## Initialize networking
ip addr add 192.168.1.2/24 dev ens33
ip route add default via 192.168.1.1

apparmor_parser -r /etc/apparmor.d/usr.sbin.proftpd
# aa-enforce /usr/sbin/proftpd
systemctl restart proftpd.service
aa-status --filter.exe=/usr/sbin/proftpd