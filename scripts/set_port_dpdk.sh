
# 导出环境变量
export PATH=$PATH:/usr/local/share/openvswitch/scripts
export DB_SOCK=/usr/local/var/run/openvswitch/db.sock

# 设置DPDK大页
dpdk-hugepages.py -p 1G --setup 16G

# 添加 neth-peer
sudo ip link add p1 type veth peer name p2 > /dev/null
ifconfig p1 100.0.0.1/24 up
ifconfig p2 100.0.1.1/24 up

