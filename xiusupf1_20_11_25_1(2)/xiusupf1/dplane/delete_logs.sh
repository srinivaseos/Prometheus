#!/bin/bash


# Function to clean up log files
cleanup_logs() {
    local directory=$1   # Directory containing log files
    local retain_count=$2 # Number of latest logs to retain

    # Change to the specified directory
    cd "$directory" || { echo "Failed to change to directory: $directory"; return; }

   # Count the number of log files (assuming they have a .log extension)
    log_files_count=$(ls -1 *.log  | wc -l)


    # Check if the number of log files exceeds the threshold
    if [ "$log_files_count" -gt "$retain_count" ]; then
    
     # Find and delete older files, keeping the latest specified number
       ls -t *.log | tail -n  +6 | xargs -I {}  rm -- {}


        echo "Old log files deleted in $directory, keeping the latest $retain_count"
    else
        echo "Number of log files in $directory is $log_files_count, which is not greater than $retain_count. No files will be deleted."
    fi
}

# Directories and number of logs to retain
cleanup_logs "/home/dpdk/xiusupf/dplane/logs/" 10
cleanup_logs "/home/dpdk/xiusupf/dplane/logs/app/" 10
cleanup_logs "/home/dpdk/xiusupf/dplane/logs/device/" 10
cleanup_logs "/home/dpdk/xiusupf/dplane/logs/l7/" 10
cleanup_logs "/home/dpdk/xiusupf/dplane/logs/perf/" 10
cleanup_logs "/home/dpdk/xiusupf/dplane/logs/pfcp/" 10
cleanup_logs "/home/dpdk/xiusupf/dplane/logs/pkt/" 10

cd /home/dpdk/xiusupf/dplane
