#!/bin/bash

## Initialize networking
ip link add name br-auth type bridge
ip link set dev br-auth type bridge vlan_filtering 1

# Add all relevant interfaces to the bridge, including the uplink (eth0)
ip link set dev eth0 master br-auth
ip link set dev eth1 master br-auth
ip link set dev eth2 master br-auth

# # TEMP: set statically VLAN-port association (#TODO: remove)
# bridge vlan add dev eth1 vid 32 pvid untagged
# bridge vlan add dev eth2 vid 95 pvid untagged
# bridge vlan add dev eth0 vid 32
# bridge vlan add dev eth0 vid 95

# # NEW: Allow the bridge CPU port itself to receive traffic on these VLANs
# bridge vlan add dev br-auth vid 32 self
# bridge vlan add dev br-auth vid 95 self

# Bring up the physical interfaces and the bridge
ip link set dev eth0 up
ip link set dev eth1 up
ip link set dev eth2 up
ip link set br-auth up

# Assign the IP address and default route to the BRIDGE interface, not eth0
ip addr add 192.168.2.2/24 dev br-auth
ip route add default via 192.168.2.1 dev br-auth

# Forward EAPOL frames (for 802.1X if you are using hostapd)
echo 8 > /sys/class/net/br-auth/bridge/group_fwd_mask
ebtables -P FORWARD DROP
ebtables -P INPUT ACCEPT
ebtables -P OUTPUT ACCEPT

## Conclude
cd
exec bash

# hostapd /etc/hostapd/hostapd.conf -B
# hostapd_cli -a 8021x_handler.py