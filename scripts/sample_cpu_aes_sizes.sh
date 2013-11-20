#!/usr/bin/env bash
# Usage: ./sample_cpu_aes_sizes.sh

cd "$(dirname "$0")"
source ./common.sh
cd $ROOT

# set -x
set -e
executable=example-aes

start_size=16
max_array_size=$((512*1024*1024))
sample_powers_of_2_using_constant_start $executable $start_size $max_array_size
