#!/bin/bash
ulimit -c unlimited
export LD_LIBRARY_PATH=.
./upf ./dpnc.json --file-prefix 13
