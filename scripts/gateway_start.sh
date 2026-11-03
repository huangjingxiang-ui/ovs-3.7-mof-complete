#!/bin/bash
ScriptPath="/root/code/multi-protocol/scripts"
cd $ScriptPath || exit

$ScriptPath/system_init.sh

$ScriptPath/ovs_start.sh

VhostPath="/root/code/multi-protocol/tcp_vhost"
cd $VhostPath||exit

$ScriptPath/tcp_vhost.sh

SocketPath="/root/code/multi-protocol/socket"
cd $SocketPath||exit

# nohup python3 server.py &

# $SocketPath/flow/ipv4_to_ipv6_flow.sh
# $SocketPath/flow/read_ccsds_flow.sh
# $SocketPath/flow/vcid_to_ipv4_flow.sh
