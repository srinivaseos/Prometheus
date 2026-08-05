#!/bin/bash

echo "Removing log files"

cd logs

#rm -rf *.log
#rm -rf app device l7 perf pfcp pkt
rm -rf /home/dpdk/new_upf/xiusupf1/dplane/logs/*.log
rm -rf /home/dpdk/new_upf/xiusupf1/dplane/logs/app/*.log
rm -rf /home/dpdk/new_upf/xiusupf1/dplane/logs/device/*.log
rm -rf /home/dpdk/new_upf/xiusupf1/dplane/logs/l7/*.log
rm -rf /home/dpdk/new_upf/xiusupf1/dplane/logs/perf/*.log
rm -rf /home/dpdk/new_upf/xiusupf1/dplane/logs/pfcp/*.log
rm -rf /home/dpdk/new_upf/xiusupf1/dplane/logs/pkt/*.log

echo "logs Removed"

cd ..

