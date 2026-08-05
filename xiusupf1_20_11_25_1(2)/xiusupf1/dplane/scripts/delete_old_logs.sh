#!/bin/bash

# Specify the directory where your log files are located
log_dir="/home/dpdk/xiusupf/dplane/logs/"

# Change to the log directory
cd "$log_dir" || exit

# Find all files, sort them by modification time (oldest first), and keep the last 10
files_to_delete=$(ls -1t | tail -n +11)

# Delete all the files except the latest 10
for file in $files_to_delete; do
    rm "$file"
    echo "Deleted: $file"
done