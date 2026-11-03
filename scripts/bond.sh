#!/bin/bash
cd /etc/sysconfig/network-scripts
mkdir net_bak
cp ifcfg-* net_bak/
ls net_bak/

nmcli connection add type bond con-name bond0 ifname bond0 mode active-backup
nmcli connection add type bond-slave ifname enp130s0f0 master bond0
nmcli connection add type bond-slave ifname enp130s0f1 master bond0

nmcli connection modify bond0 ipv4.addresses "192.168.10.1/24" ipv4.gateway "192.168.10.1"
nmcli connection modify bond0 ipv4.method manual

nmcli connection up bond0

ip addr show bond0
ip route
