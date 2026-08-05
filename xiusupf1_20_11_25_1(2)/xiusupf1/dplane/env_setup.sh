#!/bin/bash

# Check if the user is root
if [[ $EUID != 0 ]]; then
  echo "This script must be run as root."
  exit 1
fi

sudo modprobe vfio-pci
sudo modprobe vfio enable_unsafe_noiommu_mode=1
sudo echo 1 > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode
mkdir /dev/hugepages
mount -t hugetlbfs nodev /dev/hugepages/
sudo echo 12 > /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
sudo ifconfig ens224 down
sudo ifconfig ens256 down
./dpdk-devbind.py --bind=vfio-pci 0000:13:00.0
./dpdk-devbind.py --bind=vfio-pci 0000:1b:00.0
cat /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
./dpdk-devbind.py -s
