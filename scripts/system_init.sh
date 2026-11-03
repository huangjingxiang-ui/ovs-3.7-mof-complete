#!/bin/bash
# 配置大页
dpdk-hugepages.py -p 512M --setup 16G

# dpdk-devbind.py -s
ifconfig enp130s0f0 down
ifconfig enp130s0f1 down
modprobe uio_pci_generic
dpdk-devbind.py --bind=uio_pci_generic 0000:82:00.0
dpdk-devbind.py --bind=uio_pci_generic 0000:82:00.1
