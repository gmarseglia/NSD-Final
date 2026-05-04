#!/bin/bash

## Initialize networking
ip link add name br-auth type bridge

# Add all relevant interfaces to the bridge, including the uplink (eth0)
ip link set dev eth0 master br-auth
ip link set dev eth1 master br-auth
ip link set dev eth2 master br-auth

# Bring up the physical interfaces and the bridge
ip link set dev eth0 up
ip link set dev eth1 up
ip link set dev eth2 up
ip link set br-auth up

# Forward EAPOL frames (for 802.1X if you are using hostapd)
echo 8 > /sys/class/net/br-auth/bridge/group_fwd_mask

ebtables -P FORWARD DROP
ebtables -P INPUT ACCEPT
ebtables -P OUTPUT ACCEPT

# Assign the IP address and default route to the BRIDGE interface, not eth0
ip addr add 192.168.2.2/24 dev br-auth
ip route add default via 192.168.2.1 dev br-auth

## Conclude
cd
exec bash

# hostapd -d /etc/hostapd/hostapd.conf
# hostapd_cli -a 8021x_handler.py